# fcitx5-vinput 开发笔记

## 1. 项目概览

`fcitx5-vinput` 是一个基于 Fcitx5 的本地离线语音输入法。

当前交互链路是：

1. 用户在 Fcitx5 中按住触发键
2. `vinput-daemon` 开始录音
3. 用户松开按键
4. daemon 停止录音并执行离线识别
5. 识别结果通过 D-Bus 回传
6. Fcitx5 插件把文本提交到当前输入框

这个项目最终落成的是一个双进程架构：

- `fcitx5-vinput` 插件：负责按键、提示、文本提交
- `vinput-daemon`：负责录音、ASR 推理、D-Bus 服务

---

## 2. 本轮开发做了什么

这一轮开发把项目从“能编译的原型”推进到了“可以实际使用的最小可用版本”。

已经完成的工作包括：

- 用 `CMake` 搭好工程结构
- 定义插件和 daemon 共用的 D-Bus 接口
- 基于 `InputMethodEngineV2` 实现 Fcitx5 插件
- 用 `sd-bus` 实现 daemon 主循环和 D-Bus 服务
- 用 `PipeWire` 实现麦克风采集
- 接入 `sherpa-onnx` 做离线语音识别
- 增加模型目录管理
- 增加 D-Bus activation 和 `systemd --user` 服务文件
- 增加部署脚本，安装插件、daemon、service 和运行时库
- 修复一批真实运行中暴露出来的生命周期和线程问题

一句话总结：这轮不是只把代码“写完”，而是把完整链路打通并修到了可用状态。

---

## 3. 当前架构

### 3.1 总体结构

```text
Fcitx5
  └─ fcitx5-vinput.so
       ├─ 监听触发键
       ├─ 更新 preedit 提示
       ├─ 调用 D-Bus 的 `StartRecording` / `StopRecording`
       └─ 把 `RecognitionResult` 提交到输入框

D-Bus（session bus）
  └─ org.fcitx.Vinput

vinput-daemon
  ├─ D-Bus 服务（sd-bus）
  ├─ AudioCapture（PipeWire）
  ├─ AsrEngine（sherpa-onnx）
  └─ ModelManager
```

### 3.2 统一的 D-Bus 协议

插件和 daemon 共用下面这组定义：

- 总线名：`org.fcitx.Vinput`
- 对象路径：`/org/fcitx/Vinput`
- 接口名：`org.fcitx.Vinput`

方法：

- `StartRecording()`
- `StopRecording()`
- `GetStatus()`

信号：

- `RecognitionResult(string text)`
- `StatusChanged(string status)`

这个边界划得比较清楚，所以后面不管是换模型还是换 ASR 后端，都不需要重做 Fcitx5 这一层。

---

## 4. 技术选型

### 4.1 语言与构建

使用：

- `C++20`
- `CMake`

原因：

- Fcitx5 的头文件和开发方式本身就偏现代 C++
- 项目里有比较多系统资源管理场景，RAII 更合适
- `CMake` 比较适合统一链接 Fcitx5、PipeWire、systemd 和第三方库

### 4.2 Fcitx5 插件

使用：

- `fcitx::InputMethodEngineV2`
- Fcitx5 addon factory
- Fcitx5 自带 D-Bus 模块
- `InputContext`、`InputPanel`、preedit 相关 API

解决的问题：

- 挂进 Fcitx5 输入法生命周期
- 监听按键按下和松开
- 给用户显示录音中 / 推理中的提示
- 把识别结果真正提交到当前输入框

### 4.3 D-Bus

使用：

- daemon 侧：`sd-bus`
- addon 侧：Fcitx5 自带的 D-Bus 封装
- D-Bus activation 文件
- `systemd --user` 服务，`Type=dbus`

这一层的设计价值很大：

- daemon 不需要额外引入更重的 D-Bus 运行时
- addon 和 daemon 的边界清晰
- `org.fcitx.Vinput` 这个 bus name 本身就成了系统边界

### 4.4 音频采集

使用：

- `PipeWire`
- 固定录音格式：`16kHz / mono / S16_LE`

这一层目前的关键行为是：

- daemon 常驻
- ASR 模型常驻
- 麦克风 stream 只在开始录音时创建
- 停止录音后立即销毁 stream

这才是桌面“按住说话、松手识别”输入法应该有的语义。

### 4.5 离线 ASR

最初方向：

- `FunASR`

最终落地：

- `sherpa-onnx`

原因很现实：

- FunASR 的 C++ 运行时和打包成本对最小可用版本来说太重
- `sherpa-onnx` 更容易先做出本地离线、可部署、可验证的版本

当前模型目录约定：

- `~/.local/share/fcitx5-vinput/models/paraformer-zh/`

---

## 5. 关键设计决策

### 5.1 把输入法插件和识别后端拆开

这是整个项目里最重要的架构决策。

不是把所有逻辑都塞进 Fcitx5 插件里，而是：

- 插件负责 UI、按键和提交文本
- daemon 负责录音和推理
- D-Bus 作为两者之间的边界

这样做的好处是：

- 插件保持轻量
- 音频和推理问题可以独立调试
- 后续替换模型或后端时，影响范围更可控

### 5.2 模型常驻，不做冷启动

daemon 启动后会把模型初始化好并保持常驻。

收益：

- 不需要每次录音都重新加载模型
- 交互延迟更稳定

代价：

- 空闲时也会占用一部分内存

对于语音输入法这种短句、频繁触发的交互，这个取舍是对的。

### 5.3 麦克风按需创建，不常挂 stream

第一版实现虽然不是一直录音，但会在 daemon 启动后就把 PipeWire stream 建起来。

后面改成了：

- daemon 常驻
- 模型常驻
- `BeginRecording()` 时创建 stream
- `StopAndGetBuffer()` 时销毁 stream

这样做的结果是：

- 空闲时不会一直显示麦克风占用
- 更符合“按住说话”的直觉
- 对其他音频使用场景更友好

### 5.4 sherpa-onnx 改用 C API

原来接的是 sherpa-onnx 的 C++ wrapper，但初始化 recognizer 时会崩。

后来改成：

- 仍然用 sherpa-onnx
- 但 `AsrEngine` 走更底层的 C API

这样保留了功能，同时规避了 wrapper 层不稳定的问题。

---

## 6. 遇到的问题与修复

### 6.1 FunASR 对最小可用版本来说太重

问题：

- 原始目标是做一个 FunASR 的 Fcitx5 输入法
- 但在实际集成时，运行时和部署复杂度过高

处理：

- 先切到 `sherpa-onnx` 把最小可用版本跑通

结论：

- 桌面系统集成项目里，工程可落地性和模型能力同样重要

### 6.2 sherpa-onnx C++ wrapper 初始化崩溃

问题：

- daemon 早期用的是 `libsherpa-onnx-cxx-api.so`
- 初始化 recognizer 时会直接崩

处理：

- `AsrEngine` 改成 sherpa-onnx C API

结论：

- 如果一个很薄的 C++ wrapper 行为异常，回退到更稳定的 C API 往往更可靠

### 6.3 Fcitx5 没有正确显示提示

问题：

- 识别链路通了
- 但 Fcitx5 的录音 / 推理提示不稳定，甚至不显示

根因：

- 之前只更新了 `clientPreedit`
- 没有调用 `updatePreedit()`
- 也没有设置面板侧的 `preedit`

处理：

- 同时更新 `setPreedit()` 和 `setClientPreedit()`
- 调用 `updatePreedit()`
- 再刷新输入法 UI

结论：

- Fcitx5 的 UI 不是“给个字符串就会显示”，客户端侧和面板侧都要考虑

### 6.4 PipeWire wrong-context 与 daemon 停不下来

问题：

- 运行日志里出现 `called from wrong context`
- `systemctl --user restart vinput-daemon.service` 会卡住

根因：

- PipeWire stream 操作发生在错误线程
- 早期设计用的是 `pw_main_loop + std::thread`
- 但 stream 状态又从 D-Bus 线程里直接操作

处理：

- 改成 `pw_thread_loop`
- stream 的创建和销毁放到正确的 PipeWire 线程上下文里

结论：

- 线程亲和性不是“尽量遵守”，而是必须严格遵守

### 6.5 D-Bus 事件循环能转死

问题：

- daemon 收到 D-Bus 消息后可能卡在内部处理循环里
- `SIGTERM` 也退不出来

根因：

- `sd_bus_process()` 返回值被误解了
- `> 0` 才表示处理了一条消息
- `0` 只是“当前没有更多工作”
- 之前把 `0` 也当成了继续循环

处理：

- `ProcessOnce()` 改成只在 `ret > 0` 时返回真

结论：

- 事件循环的返回值本身就是控制流，不是普通状态码

---

## 7. 验证结果

这一轮没有停留在代码层，而是做了真实运行时验证。

已经验证过的内容包括：

- daemon 可以成功初始化 ASR
- daemon 可以成功注册 D-Bus 服务
- Fcitx5 提示在修复 preedit 之后能正确显示
- PipeWire stream 只会在开始录音时创建
- 推理完成后状态可以回到 `idle`
- 录音一次之后 daemon 仍然可以正常退出
- `systemctl --user restart vinput-daemon.service` 已恢复正常

这里比较关键的一点是：

- 之前看起来像“restart 起不来”
- 实际根因是 stop 阶段退不出来

这类问题如果只看表象，很容易误判成冷启动或模型加载失败。

---

## 8. 模型优化与性能对比

### 8.1 当前模型选择

最近这一轮没有改架构，而是先优化了模型选择。

当前启用模型：

- `sherpa-onnx-paraformer-zh-small-2024-03-09`
- `model.int8.onnx` 体积约 `79M`

之前使用的模型：

- 同目录下旧的 `model.int8.onnx`
- 实际体积约 `233M`

旧模型已保留时间戳备份，方便继续做 A/B 对比。

### 8.2 固定样本基准测试

为了避免只靠主观体感，新增了一个本地基准工具：

- [asr_bench.cpp](/home/xifan/code/fcitx5-vinput/tools/asr_bench.cpp)

它直接读取模型和 `wav` 文件，绕过 Fcitx5 与 D-Bus，只量化：

- 初始化耗时
- 推理耗时
- 实时率（RTF）
- 进程常驻内存（RSS）
- 识别文本

测试样本使用官方 small 模型包自带的 4 条固定音频：

- `0.wav`
- `1.wav`
- `2-zh-en.wav`
- `3-sichuan.wav`

### 8.3 small 模型 vs 旧模型

| 指标 | small 79M | 旧模型 233M |
| --- | ---: | ---: |
| 初始化耗时 | `3500 ms` | `4887 ms` |
| 初始化后常驻内存（RSS） | `131652 kB` | `299912 kB` |
| 4 条样本平均推理耗时 | `1545 ms` | `3175 ms` |
| 平均实时率（RTF） | `0.236` | `0.476` |

可以直接得出的结论是：

- small 模型平均推理大约快了 `2.05x`
- 初始化更快，约快了 `28%`
- 初始化后内存显著下降

### 8.4 识别效果观察

在这 4 条固定样本里：

- `1.wav`、`2-zh-en.wav`、`3-sichuan.wav` 的识别文本和旧模型一致
- `0.wav` 只有结尾词有轻微差异

这说明至少在这组样本下：

- small 模型没有出现明显质量退化
- 但速度和内存收益非常明显

### 8.5 实际运行中的 daemon 内存采样

在真实运行中的 `vinput-daemon` 上，重新测了一次小模型版本的内存：

- 空闲：`VmRSS 137236 kB`
- 录音中：`VmRSS 139568 kB`
- `StopRecording` 推理阶段采样峰值：`VmRSS 139828 kB`
- 推理结束后：`VmRSS 137556 kB`
- `VmSwap` 始终为 `0`

另外还测了一次真实调用链：

- `StartRecording` D-Bus 调用返回耗时约 `19 ms`
- 录音 2 秒后，`StopRecording` 阻塞返回约 `557 ms`

和前面那版大模型相比，当前小模型版本的优势很明显：

- 常驻内存从大约 `300MB+` 下降到了 `130MB+`
- 松手后的出字等待时间明显缩短

结论：

- 对当前这个桌面语音输入法场景，小模型是更合适的默认选项

---

## 9. 当前项目状态

截至这轮开发，项目的状态可以概括为：

- 本地离线语音输入可用
- Fcitx5 插件和 daemon 的边界清晰
- D-Bus 协议稳定且简单
- “按住说话、松手识别”的流程已经跑通
- 麦克风生命周期正确
- daemon 的启动、停止、重启都已稳定
- 默认模型已经切到更适合日常输入的 small 版本

它还不是一个完全打磨好的产品，但已经不是“只能演示的原型”了。

---

## 10. 这轮开发的主要经验

这个项目很像一个典型的 Linux 桌面集成工程，最大的难点不是“算法”，而是“系统行为”。

这一轮最值得记住的几点是：

- 尽早把进程边界设计清楚
- 优先使用稳定、边界清晰的 API
- 不要只验证能不能编译，要验证真实运行时行为
- 停止、重启、清理资源和启动同样重要
- 很多看似“启动失败”的问题，根因其实在 stop / teardown
- 语音输入这种交互里，模型选择往往比继续堆代码更有性价比

---

## 11. 下一步建议

后面比较值得继续做的方向有：

- 给 addon 增加更明确的错误提示
- 让触发键可配置
- 增加录音超时和取消逻辑
- 补安装说明、模型说明和排障说明
- 如果后面想继续提升能力，再评估新的小模型或 `SenseVoice`

短期内最务实的策略不是大改架构，而是：

- 保持现在这套 Fcitx5 + D-Bus + daemon 结构
- 继续通过模型选择来优化体验

---

## 12. 总结

这一轮开发真正完成的，不只是“写了一个插件”。

我们实际做的是：

- 搭出输入法与识别后端分离的整体架构
- 打通 Fcitx5、D-Bus、PipeWire 和离线 ASR
- 修掉真实桌面环境中的线程和生命周期问题
- 把录音行为改成真正的“按住说话、松手识别”
- 通过更合适的小模型，把速度和内存显著压下来

如果把这份文档当成学习笔记来读，最核心的一句话其实是：

> 一个能用的桌面语音输入法，关键不只是识别模型本身，更在于进程边界、事件循环、线程上下文、资源释放，以及对真实运行时行为的验证。
