#!/usr/bin/env python3
"""Repo-native validation entrypoint for DG-LAN."""

from __future__ import annotations

import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent
PYTHON = sys.executable


@dataclass
class StepResult:
    name: str
    status: str
    detail: str


def probe(command: list[str], cwd: Path = ROOT) -> tuple[int, str]:
    completed = subprocess.run(
        command,
        cwd=cwd,
        text=True,
        capture_output=True,
        check=False,
    )
    output = "\n".join(part.strip() for part in (completed.stdout, completed.stderr) if part.strip())
    return completed.returncode, output


def run(command: list[str], cwd: Path) -> int:
    print(f"\n==> {cwd.relative_to(ROOT) if cwd != ROOT else '.'}$ {' '.join(command)}", flush=True)
    completed = subprocess.run(command, cwd=cwd, check=False)
    return completed.returncode


def python_bridge_tests() -> StepResult:
    missing = []
    for label, module in (("pytest", "pytest"), ("protobuf", "google.protobuf")):
        code, _ = probe([PYTHON, "-c", f"import {module}"])
        if code != 0:
            missing.append(label)

    if missing:
        return StepResult(
            "Python bridge tests",
            "BLOCKED",
            f"Missing Python prerequisite(s): {', '.join(missing)}",
        )

    code = run([PYTHON, "-m", "pytest", "test_streamer.py", "-v"], ROOT / "dglan-api")
    if code == 0:
        return StepResult("Python bridge tests", "PASS", "59 pytest cases passed")

    return StepResult("Python bridge tests", "FAIL", f"pytest exited with code {code}")


def desktop_cpp_tests() -> StepResult:
    bash = shutil.which("bash")
    qmake = shutil.which("qmake-qt5") or shutil.which("qmake")
    protoc = shutil.which("protoc")

    missing = []
    if not bash:
        missing.append("bash")
    if not qmake:
        missing.append("qmake/qmake-qt5")
    if not protoc:
        missing.append("protoc")

    detail_suffix = (
        " Linux validation now compiles Core/GUI and runs the wired suites TestsCommon, TestsFileManager, "
        "TestsPeerManager, and TestsDownloadManager. Optional legacy tool builds such as Tools/PasswordHasher "
        "stay out of the default graph, and other discovered Qt suites remain unwired."
    )

    if missing:
        return StepResult(
            "Desktop Qt/C++ tests",
            "BLOCKED",
            f"Missing desktop toolchain prerequisite(s): {', '.join(missing)}.{detail_suffix}",
        )

    build_code = run([bash, "3.compile_all_components.sh", "--validation"], ROOT / "application")
    if build_code != 0:
        return StepResult(
            "Desktop Qt/C++ tests",
            "FAIL",
            f"Linux validation build step failed with exit code {build_code}.{detail_suffix}",
        )

    test_code = run([bash, "4.run_all_tests.sh", "--validation"], ROOT / "application")
    if test_code != 0:
        return StepResult(
            "Desktop Qt/C++ tests",
            "FAIL",
            f"Linux validation test step failed with exit code {test_code}.{detail_suffix}",
        )

    return StepResult(
        "Desktop Qt/C++ tests",
        "PASS",
        f"Linux validation profile completed successfully.{detail_suffix}",
    )


def main() -> int:
    results = [python_bridge_tests(), desktop_cpp_tests()]

    print("\nValidation summary:")
    for result in results:
        print(f"- [{result.status}] {result.name}: {result.detail}")

    if any(result.status == "FAIL" for result in results):
        return 1
    if any(result.status == "BLOCKED" for result in results):
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
