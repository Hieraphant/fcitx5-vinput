#!/usr/bin/env python3
# ==vinput-asr-provider==
# @name OpenAI-Compatible Speech to Text
# @description 通过 OpenAI 兼容 /v1/audio/transcriptions 接口调用云端 ASR
# @author xifan
# @version 1.0.0
# @env OPENAI_COMPATIBLE_ASR_API_KEY (required)
# @env OPENAI_COMPATIBLE_ASR_URL (optional, default: https://api.openai.com/v1/audio/transcriptions)
# @env OPENAI_COMPATIBLE_ASR_BASE_URL (optional, used when OPENAI_COMPATIBLE_ASR_URL is unset)
# @env OPENAI_COMPATIBLE_ASR_MODEL (optional, default: whisper-1)
# @env OPENAI_COMPATIBLE_ASR_LANGUAGE (optional)
# @env OPENAI_COMPATIBLE_ASR_PROMPT (optional)
# @env OPENAI_COMPATIBLE_ASR_RESPONSE_FORMAT (optional, default: json)
# @env OPENAI_COMPATIBLE_ASR_TEMPERATURE (optional)
# @env OPENAI_COMPATIBLE_ASR_TIMEOUT (optional)
# ==/vinput-asr-provider==
"""Built-in OpenAI-compatible ASR provider for vinput.

Reads one complete PCM S16_LE 16 kHz mono utterance from stdin, sends it to an
OpenAI-compatible /v1/audio/transcriptions endpoint, and writes the final
transcript to stdout. Errors are written to stderr only.
"""

import io
import json
import os
import sys
import uuid
import wave
from typing import Iterable, Optional, Tuple
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

DEFAULT_URL = "https://api.openai.com/v1/audio/transcriptions"
DEFAULT_MODEL = "whisper-1"
DEFAULT_RESPONSE_FORMAT = "json"
DEFAULT_TIMEOUT = 60
EXIT_RUNTIME_ERROR = 1
EXIT_USAGE_ERROR = 2


def get_required_env(name: str) -> str:
    value = os.getenv(name, "").strip()
    if not value:
        raise ValueError(f"Missing {name}.")
    return value


def get_optional_env(name: str, default: str = "") -> str:
    value = os.getenv(name, "").strip()
    return value or default


def get_optional_int_env(name: str, default: int) -> int:
    value = os.getenv(name, "").strip()
    if not value:
        return default
    return int(value)


def read_audio_input() -> bytes:
    pcm_audio = sys.stdin.buffer.read()
    if not pcm_audio:
        raise ValueError("No audio received on stdin.")
    return pcm_audio


def pcm_to_wav_bytes(pcm_audio: bytes) -> bytes:
    buffer = io.BytesIO()
    with wave.open(buffer, "wb") as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(16000)
        wav_file.writeframes(pcm_audio)
    return buffer.getvalue()


def build_multipart(
    fields: Iterable[Tuple[str, str]],
    files: Iterable[Tuple[str, str, str, bytes]],
) -> Tuple[bytes, str]:
    boundary = f"----vinput-{uuid.uuid4().hex}"
    body = bytearray()

    for name, value in fields:
        body.extend(f"--{boundary}\r\n".encode())
        body.extend(
            f'Content-Disposition: form-data; name="{name}"\r\n\r\n'.encode()
        )
        body.extend(value.encode())
        body.extend(b"\r\n")

    for field_name, filename, content_type, content in files:
        body.extend(f"--{boundary}\r\n".encode())
        body.extend(
            (
                f'Content-Disposition: form-data; name="{field_name}"; '
                f'filename="{filename}"\r\n'
            ).encode()
        )
        body.extend(f"Content-Type: {content_type}\r\n\r\n".encode())
        body.extend(content)
        body.extend(b"\r\n")

    body.extend(f"--{boundary}--\r\n".encode())
    return bytes(body), boundary


def parse_error_payload(payload: bytes) -> str:
    text = payload.decode("utf-8", errors="replace").strip()
    if not text:
        return "Empty error response."

    try:
        data = json.loads(text)
    except json.JSONDecodeError:
        return text

    if isinstance(data, dict):
        error = data.get("error")
        if isinstance(error, dict):
            message = error.get("message")
            if isinstance(message, str) and message.strip():
                return message.strip()
        if isinstance(error, str) and error.strip():
            return error.strip()
        message = data.get("message")
        if isinstance(message, str) and message.strip():
            return message.strip()

    return text


def parse_transcript(payload: bytes, response_format: str) -> str:
    text_payload = payload.decode("utf-8", errors="replace").strip()
    if not text_payload:
        raise RuntimeError("ASR service returned an empty response.")

    if response_format in {"text", "srt", "vtt"}:
        return text_payload

    data = json.loads(text_payload)
    if not isinstance(data, dict):
        raise RuntimeError("ASR service returned an unexpected response.")

    text = data.get("text")
    if not isinstance(text, str) or not text.strip():
        raise RuntimeError("ASR service returned an empty transcript.")
    return text.strip()


def resolve_endpoint() -> str:
    explicit_url = get_optional_env("OPENAI_COMPATIBLE_ASR_URL")
    if explicit_url:
        return explicit_url

    base_url = get_optional_env("OPENAI_COMPATIBLE_ASR_BASE_URL")
    if base_url:
        return base_url.rstrip("/") + "/audio/transcriptions"

    return DEFAULT_URL


def transcribe(
    pcm_audio: bytes,
    api_key: str,
    endpoint: str,
    model: str,
    response_format: str,
    timeout: int,
    language: Optional[str],
    prompt: Optional[str],
    temperature: Optional[str],
) -> str:
    wav_audio = pcm_to_wav_bytes(pcm_audio)

    fields = [("model", model), ("response_format", response_format)]
    if language:
        fields.append(("language", language))
    if prompt:
        fields.append(("prompt", prompt))
    if temperature:
        fields.append(("temperature", temperature))

    body, boundary = build_multipart(
        fields=fields,
        files=[("file", "audio.wav", "audio/wav", wav_audio)],
    )

    request = Request(
        endpoint,
        data=body,
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type": f"multipart/form-data; boundary={boundary}",
            "Accept": "application/json, text/plain;q=0.9, */*;q=0.8",
        },
        method="POST",
    )

    with urlopen(request, timeout=timeout) as response:
        return parse_transcript(response.read(), response_format)


def main() -> int:
    try:
        api_key = get_required_env("OPENAI_COMPATIBLE_ASR_API_KEY")
        endpoint = resolve_endpoint()
        model = get_optional_env("OPENAI_COMPATIBLE_ASR_MODEL", DEFAULT_MODEL)
        response_format = get_optional_env(
            "OPENAI_COMPATIBLE_ASR_RESPONSE_FORMAT",
            DEFAULT_RESPONSE_FORMAT,
        )
        timeout = get_optional_int_env(
            "OPENAI_COMPATIBLE_ASR_TIMEOUT", DEFAULT_TIMEOUT
        )
        language = get_optional_env("OPENAI_COMPATIBLE_ASR_LANGUAGE") or None
        prompt = get_optional_env("OPENAI_COMPATIBLE_ASR_PROMPT") or None
        temperature = (
            get_optional_env("OPENAI_COMPATIBLE_ASR_TEMPERATURE") or None
        )
        pcm_audio = read_audio_input()

        text = transcribe(
            pcm_audio=pcm_audio,
            api_key=api_key,
            endpoint=endpoint,
            model=model,
            response_format=response_format,
            timeout=timeout,
            language=language,
            prompt=prompt,
            temperature=temperature,
        )
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return EXIT_USAGE_ERROR
    except HTTPError as exc:
        payload = exc.read()
        print(
            f"OpenAI-compatible ASR HTTP {exc.code}: "
            f"{parse_error_payload(payload)}",
            file=sys.stderr,
        )
        return EXIT_RUNTIME_ERROR
    except URLError as exc:
        print(
            f"Failed to reach OpenAI-compatible ASR service: {exc}",
            file=sys.stderr,
        )
        return EXIT_RUNTIME_ERROR
    except Exception as exc:
        print(str(exc), file=sys.stderr)
        return EXIT_RUNTIME_ERROR

    sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
