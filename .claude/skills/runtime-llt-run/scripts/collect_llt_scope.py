#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------
"""Collect Runtime LLT impact scope for helper diagnostics.

This script is intentionally not the Runtime LLT gate entry. Use
runtime_llt_select_and_run.py for build, run, coverage, rerun, and report.
"""

from __future__ import annotations

import argparse
import bisect
import contextlib
import json
import logging
import os
import re
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path


IDENT_RE = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]{2,}\b")
CALL_RE = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")
TEST_RE = re.compile(
    r"\bTEST(?:_F|_P)?\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)"
)

STOP_WORDS = {
    "and",
    "api",
    "assert",
    "ASSERT_EQ",
    "ASSERT_FALSE",
    "ASSERT_NE",
    "ASSERT_TRUE",
    "auto",
    "bool",
    "case",
    "char",
    "class",
    "const",
    "constexpr",
    "continue",
    "creating",
    "default",
    "delete",
    "define",
    "double",
    "else",
    "enum",
    "error",
    "event",
    "events",
    "EXPECT_EQ",
    "EXPECT_FALSE",
    "EXPECT_NE",
    "EXPECT_TRUE",
    "false",
    "flag",
    "float",
    "for",
    "if",
    "include",
    "inner",
    "inline",
    "int",
    "long",
    "memory",
    "mode",
    "namespace",
    "new",
    "nullptr",
    "out",
    "param",
    "params",
    "passed",
    "private",
    "protected",
    "public",
    "record",
    "ret",
    "return",
    "reset",
    "rtError_t",
    "rtStream_t",
    "short",
    "signed",
    "sizeof",
    "software",
    "hardware",
    "static",
    "static_cast",
    "std",
    "string",
    "struct",
    "switch",
    "task",
    "TaskInfo",
    "taskInfo",
    "taskinfo",
    "test",
    "TEST_F",
    "TEST_P",
    "template",
    "the",
    "this",
    "true",
    "type",
    "typedef",
    "typename",
    "uint32_t",
    "uint64_t",
    "unsigned",
    "using",
    "wait",
    "when",
    "void",
    "while",
}

LOW_SIGNAL_PREFIXES = (
    "ACL_ERROR_",
    "ERROR_RETURN",
    "NULL_RETURN",
    "PARAM_",
    "RT_ERROR_",
    "RT_LOG_",
    "UNUSED",
)

LOGGER = logging.getLogger(__name__)
GIT_EXECUTABLE = shutil.which("git") or "/usr/bin/git"


def run_git(args: list[str], check: bool = True) -> str:
    proc = subprocess.run(
        [GIT_EXECUTABLE, *args],
        check=False,
        text=True,
        capture_output=True,
    )
    if check and proc.returncode != 0:
        raise RuntimeError(proc.stderr.strip() or f"git {' '.join(args)} failed")
    return proc.stdout


def default_base() -> str:
    for ref in ("origin/master", "origin/main", "master", "main"):
        try:
            base = run_git(["merge-base", "HEAD", ref]).strip()
            if base:
                return base
        except RuntimeError:
            continue
    return run_git(["rev-parse", "HEAD~1"]).strip()


def changed_files(base: str, include_worktree: bool) -> list[str]:
    files: set[str] = set()
    for rev in (f"{base}...HEAD", f"{base}..HEAD"):
        out = run_git(["diff", "--name-only", "--diff-filter=ACMRD", rev], check=False)
        if out:
            files.update(line.strip() for line in out.splitlines() if line.strip())
            break
    if include_worktree:
        for args in (
            ["diff", "--name-only", "--diff-filter=ACMRD"],
            ["diff", "--cached", "--name-only", "--diff-filter=ACMRD"],
            ["ls-files", "--others", "--exclude-standard"],
        ):
            out = run_git(args, check=False)
            files.update(line.strip() for line in out.splitlines() if line.strip())
    return sorted(files)


def added_lines_from_diff(args: list[str]) -> list[str]:
    out = run_git(args, check=False)
    lines: list[str] = []
    for line in out.splitlines():
        if line.startswith("+") and not line.startswith("+++"):
            lines.append(line[1:])
    return lines


def is_low_signal(ident: str) -> bool:
    if ident in STOP_WORDS or ident.lower() in STOP_WORDS:
        return True
    if ident.startswith("__"):
        return True
    if ident.startswith(LOW_SIGNAL_PREFIXES):
        return True
    if ident.isupper() and ("ERROR" in ident or ident.endswith("_NONE")):
        return True
    return False


def diff_texts_for_file(base: str, path: str, include_worktree: bool) -> list[str]:
    texts: list[str] = []
    for rev in (f"{base}...HEAD", f"{base}..HEAD"):
        out = run_git(["diff", "-U0", rev, "--", path], check=False)
        if out:
            texts.append(out)
            break
    if include_worktree:
        for args in (
            ["diff", "-U0", "--", path],
            ["diff", "--cached", "-U0", "--", path],
        ):
            out = run_git(args, check=False)
            if out:
                texts.append(out)
    return texts


def primary_symbols_from_diff_text(diff_text: str) -> list[str]:
    symbols: list[str] = []
    for line in diff_text.splitlines():
        if line.startswith("@@"):
            context = line.rsplit("@@", 1)[-1]
            for ident in CALL_RE.findall(context):
                if not is_low_signal(ident) and ident not in symbols:
                    symbols.append(ident)
        elif line.startswith("+") and not line.startswith("+++"):
            for ident in CALL_RE.findall(line[1:]):
                if not is_low_signal(ident) and ident not in symbols:
                    symbols.append(ident)
    return symbols


def collect_diff_material(
    base: str,
    files: list[str],
    include_worktree: bool,
) -> tuple[list[str], list[str]]:
    lines: list[str] = []
    primary_symbols: list[str] = []
    for path in files:
        before = len(lines)
        diff_texts = diff_texts_for_file(base, path, include_worktree)
        for text in diff_texts:
            for symbol in primary_symbols_from_diff_text(text):
                if symbol not in primary_symbols:
                    primary_symbols.append(symbol)
            for line in text.splitlines():
                if line.startswith("+") and not line.startswith("+++"):
                    lines.append(line[1:])
        if len(lines) == before and Path(path).is_file():
            with contextlib.suppress(OSError):
                lines.extend(Path(path).read_text(errors="ignore").splitlines())
    return primary_symbols, lines


def symbols_from(
    primary_symbols: list[str],
    lines: list[str],
    files: list[str],
    limit: int,
) -> list[str]:
    counter: Counter[str] = Counter()
    for line in lines:
        for ident in IDENT_RE.findall(line):
            if is_low_signal(ident):
                continue
            counter[ident] += 1

    for path in files:
        stem = Path(path).stem
        for part in re.split(r"[^A-Za-z0-9_]+|_", stem):
            if len(part) >= 3 and not is_low_signal(part):
                counter[part] += 1

    ordered = list(primary_symbols)
    for name, _ in counter.most_common(limit):
        if name not in ordered:
            ordered.append(name)
    return ordered[:limit]


def line_starts(text: str) -> list[int]:
    starts = [0]
    starts.extend(match.end() for match in re.finditer(r"\n", text))
    return starts


def byte_to_line(starts: list[int], offset: int) -> int:
    return bisect.bisect_right(starts, offset)


def collect_tests(test_root: Path, symbols: list[str]) -> list[dict[str, object]]:
    if not test_root.exists():
        return []

    symbol_patterns = {
        symbol: re.compile(rf"\b{re.escape(symbol)}\b") for symbol in symbols
    }
    tests: dict[str, dict[str, object]] = {}

    for root, _, filenames in os.walk(test_root):
        for filename in filenames:
            if not filename.endswith((".cc", ".cpp", ".cxx")):
                continue
            path = Path(root) / filename
            try:
                text = path.read_text(errors="ignore")
            except OSError:
                continue

            matches = list(TEST_RE.finditer(text))
            if not matches:
                continue

            starts = line_starts(text)
            for index, match in enumerate(matches):
                begin = match.start()
                end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
                body = text[begin:end]
                hit_symbols = [
                    symbol for symbol, pattern in symbol_patterns.items() if pattern.search(body)
                ]
                if not hit_symbols:
                    continue
                suite, case = match.group(1), match.group(2)
                name = f"{suite}.{case}"
                existing = tests.get(name)
                payload = {
                    "name": name,
                    "file": str(path),
                    "line": byte_to_line(starts, match.start()),
                    "symbols": hit_symbols[:20],
                }
                if existing is None or len(hit_symbols) > len(existing["symbols"]):
                    tests[name] = payload

    return [tests[name] for name in sorted(tests)]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", default="auto", help="git base ref for diff")
    parser.add_argument(
        "--test-root",
        default="tests/ut/runtime/runtime/test",
        help="runtime UT source directory",
    )
    parser.add_argument("--out", default=None, help="write JSON result to this path")
    parser.add_argument("--max-symbols", type=int, default=120)
    parser.add_argument(
        "--committed-only",
        action="store_true",
        help="ignore uncommitted and staged changes",
    )
    args = parser.parse_args()

    base = default_base() if args.base in ("", "auto") else args.base
    include_worktree = not args.committed_only
    files = changed_files(base, include_worktree)
    primary_symbols, added_lines = collect_diff_material(base, files, include_worktree)
    symbols = symbols_from(primary_symbols, added_lines, files, args.max_symbols)
    tests = collect_tests(Path(args.test_root), symbols)

    result = {
        "base": base,
        "head": run_git(["rev-parse", "HEAD"]).strip(),
        "include_worktree": include_worktree,
        "changed_files": files,
        "primary_symbols": primary_symbols,
        "symbols": symbols,
        "test_root": args.test_root,
        "tests": tests,
        "gtest_filter": ":".join(test["name"] for test in tests),
    }

    if args.out:
        out_path = Path(args.out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")

    LOGGER.info("base: %s", result["base"])
    LOGGER.info("changed_files: %s", len(files))
    LOGGER.info("symbols: %s", len(symbols))
    LOGGER.info("tests: %s", len(tests))
    if result["gtest_filter"]:
        LOGGER.info("gtest_filter: %s", result["gtest_filter"])
    else:
        LOGGER.info("gtest_filter: <empty>")
    return 0


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    try:
        sys.exit(main())
    except RuntimeError as exc:
        LOGGER.error("error: %s", exc)
        sys.exit(1)
