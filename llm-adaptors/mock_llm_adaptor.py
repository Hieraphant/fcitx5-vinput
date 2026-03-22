#!/usr/bin/env python3
# ==vinput-adaptor==
# @name Mock LLM Adaptor
# @description Fixed-response OpenAI-compatible test server
# @author xifan
# @version 1.0.0
# @env VINPUT_MOCK_PORT (optional, default: 18080)
# @env VINPUT_MOCK_HOST (optional, default: 127.0.0.1)
# @env VINPUT_MOCK_MODEL (optional, default: mock-llm)
# @env VINPUT_MOCK_RESPONSE (optional, default: [MOCK] test response)
# ==/vinput-adaptor==
"""A minimal OpenAI-compatible mock server for local testing."""

import json
import os
import time
import uuid
from http.server import BaseHTTPRequestHandler, HTTPServer

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 18080
DEFAULT_MODEL = "mock-llm"
DEFAULT_RESPONSE = "[MOCK] test response"


def chat_response(model: str, content: str) -> dict:
    wrapped = json.dumps({"candidates": [content]})
    return {
        "id": f"chatcmpl-{uuid.uuid4().hex[:12]}",
        "object": "chat.completion",
        "created": int(time.time()),
        "model": model,
        "choices": [
            {
                "index": 0,
                "message": {"role": "assistant", "content": wrapped},
                "finish_reason": "stop",
            }
        ],
        "usage": {"prompt_tokens": 0, "completion_tokens": 0, "total_tokens": 0},
    }


class MockHandler(BaseHTTPRequestHandler):
    server_version = "vinput-mock-llm/1.0"

    def do_GET(self):
        if self.path.rstrip("/") not in ("/v1/models", "/models"):
            self.send_error(404)
            return

        response = {
            "object": "list",
            "data": [
                {
                    "id": self.server.mock_model,
                    "object": "model",
                    "owned_by": "vinput-mock",
                }
            ],
        }
        self._write_json(200, response)

    def do_POST(self):
        if self.path.rstrip("/") not in ("/v1/chat/completions", "/chat/completions"):
            self.send_error(404)
            return

        length = int(self.headers.get("Content-Length", "0"))
        payload = {}
        if length > 0:
            payload = json.loads(self.rfile.read(length))

        model = payload.get("model") or self.server.mock_model
        response = chat_response(model, self.server.mock_response)
        self._write_json(200, response)

    def log_message(self, fmt, *args):
        return

    def _write_json(self, status: int, payload: dict):
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def main():
    host = os.getenv("VINPUT_MOCK_HOST", DEFAULT_HOST).strip() or DEFAULT_HOST
    port = int(os.getenv("VINPUT_MOCK_PORT", str(DEFAULT_PORT)))
    model = os.getenv("VINPUT_MOCK_MODEL", DEFAULT_MODEL).strip() or DEFAULT_MODEL
    response = (
        os.getenv("VINPUT_MOCK_RESPONSE", DEFAULT_RESPONSE).strip()
        or DEFAULT_RESPONSE
    )

    server = HTTPServer((host, port), MockHandler)
    server.mock_model = model
    server.mock_response = response

    print(
        f"[mock-llm-adaptor] Listening on http://{host}:{port} "
        f"(model={model!r}, response={response!r})",
        flush=True,
    )
    server.serve_forever()


if __name__ == "__main__":
    main()
