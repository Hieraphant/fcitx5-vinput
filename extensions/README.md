# Extensions

This directory contains optional integration scripts that extend `fcitx5-vinput`
without becoming part of the core daemon implementation.

- `extensions/asr/`: external ASR provider scripts
- `extensions/llm/`: LLM/provider bridge scripts

These scripts are intended to be referenced by provider configuration entries.
They should follow the same contract used by the command ASR provider:

- `stdin`: one utterance audio stream from vinput
- `stdout`: final text result
- `stderr`: human-readable error output
- exit code `0`: success
- non-zero exit code: failure
