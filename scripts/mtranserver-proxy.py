#!/usr/bin/env python3
"""Compatibility wrapper for the moved MTranServer extension script."""

from pathlib import Path
import runpy


if __name__ == "__main__":
    target = (
        Path(__file__).resolve().parents[1]
        / "extensions"
        / "llm"
        / "mtranserver_proxy.py"
    )
    runpy.run_path(str(target), run_name="__main__")
