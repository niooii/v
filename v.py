#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

try:
    import arguably
except Exception as exc:  # pragma: no cover
    print(
        "arguably is required. Install with 'uv sync' or 'pip install arguably'",
        file=sys.stderr,
    )
    raise


ROOT = Path(__file__).resolve().parent
BUILD_DIR_DEBUG = ROOT / "cmake-build-debug"
BUILD_DIR_RELEASE = ROOT / "cmake-build-release"


def _preset(release: bool) -> str:
    return "release" if release else "debug"


def _build_preset(target: str | None, release: bool) -> str:
    base = _preset(release)
    if target:
        return f"{base}-{target}"
    return f"{base}-build"


def _build_dir(release: bool) -> Path:
    return BUILD_DIR_RELEASE if release else BUILD_DIR_DEBUG


def _run(
    cmd: list[str], cwd: Path | None = None, env: dict[str, str] | None = None
) -> None:
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, env=env, check=True)


def _configure_cmake(release: bool) -> None:
    preset = _preset(release)
    cmd = ["cmake", "--preset", preset]
    vcpkg = os.environ.get("VCPKG_ROOT")
    if vcpkg:
        cmd += ["-D", f"CMAKE_TOOLCHAIN_FILE={vcpkg}/scripts/buildsystems/vcpkg.cmake"]
    print(f"[configure] {' '.join(cmd)}")
    _run(cmd)


def _ensure_configured(release: bool) -> None:
    bdir = _build_dir(release)
    if not (bdir.exists() and (bdir / "build.ninja").exists()):
        _configure_cmake(release)


@arguably.command
def build(
    target: str | None = None,
    *,
    release: bool = False,
    verbose: bool = False,
    full: bool = False,
) -> None:
    """Build targets via CMake presets

    :param target: Optional target (vclient, vserver, vlib). If omitted, builds all
    :param release: Build in Release mode (default Debug)
    :param full: Show full build output (not just fatal)
    """
    _ensure_configured(release)
    preset = _build_preset(target, release)
    print(f"[build] cmake --build --preset {preset}")
    print("Starting build")
    env = os.environ.copy()
    if verbose:
        env["V_LOG_LEVEL"] = "trace"

    if full:
        _run(["cmake", "--build", "--preset", preset], env=env)
        return

    import subprocess as _sp

    proc = _sp.Popen(
        ["cmake", "--build", "--preset", preset],
        env=env,
        stdout=_sp.PIPE,
        stderr=_sp.STDOUT,
        text=True,
    )
    assert proc.stdout is not None
    lines: list[str] = []
    for line in proc.stdout:
        lines.append(line)
    rc = proc.wait()

    # grep simulation (windows maynot have)
    key = "error: "
    idxs: list[int] = []
    for i, l in enumerate(lines):
        if key in l.lower():
            idxs.append(i)

    if idxs:
        print("[build] errors:")
        shown = set()
        for i in idxs:
            start = max(0, i - 5)
            end = min(len(lines), i + 6)
            for j in range(start, end):
                if j in shown:
                    continue
                print(lines[j], end="")
                shown.add(j)
            print("----")
    else:
        print("[build] Completed without errors")

    if rc != 0:
        # If build failed, also echo the last 50 lines to aid debugging
        tail = "".join(lines[-50:]) if lines else ""
        if tail:
            print("[build] last 50 lines:")
            print(tail, end="")
        raise SystemExit(rc)


def _exe_name(name: str) -> str:
    if os.name == "nt":
        return f"{name}.exe"
    return name


@arguably.command
def run(
    target: str,
    *,
    release: bool = False,
    verbose: bool = False,
    full: bool = False,
) -> None:
    """Run an executable target (vclient or vserver)

    :param target: vclient or vserver
    :param release: Use Release build dir
    :param full: Show full build output (not just fatal)
    """
    if target not in {"vclient", "vserver"}:
        raise arguably.ArgumentError("target must be 'vclient' or 'vserver'")
    build(target=target, release=release, full=full, verbose=verbose)
    exe = _build_dir(release) / _exe_name(target)
    if not exe.exists():
        raise FileNotFoundError(f"Executable not found: {exe}")
    print(f"[run] {exe}")
    env = os.environ.copy()
    if verbose:
        env["V_LOG_LEVEL"] = "trace"
    _run([str(exe)], cwd=_build_dir(release), env=env)


@arguably.command
def clean(*, all: bool = False, release: bool = False, verbose: bool = False) -> None:
    """Clean build directory

    :param all: If true, remove the entire build directory; else partial clean
    :param release: Clean release build dir instead of debug
    """
    bdir = _build_dir(release)
    if not bdir.exists():
        print(f"[clean] build dir does not exist: {bdir}")
        return
    if all:
        print(f"[clean] Removing {bdir}")
        shutil.rmtree(bdir)
    else:
        print(f"[clean] Removing contents of {bdir}")
        for entry in bdir.iterdir():
            # keep extern downloads if present
            if entry.name in {"extern"}:
                continue
            if entry.is_dir():
                shutil.rmtree(entry)
            else:
                entry.unlink(missing_ok=True)


@arguably.command
def reload(*, release: bool = False, verbose: bool = False) -> None:
    """Reconfigure CMake cache for the current preset"""
    env = os.environ.copy()
    if verbose:
        env["V_LOG_LEVEL"] = "trace"
    _configure_cmake(release)


def _collect_files(target: str | None) -> list[Path]:
    exts = (".cpp", ".hpp", ".h", ".c")
    roots: list[Path]
    if target == "vclient":
        roots = [ROOT / "client"]
    elif target == "vserver":
        roots = [ROOT / "server"]
    elif target == "vlib":
        roots = [ROOT / "common"]
    else:
        roots = [ROOT]

    def _is_excluded(p: Path) -> bool:
        # Exclude any path inside extern/, cmake-build-*/ or .idea/ or .venv/
        for part in p.parts:
            if (
                part == "extern"
                or part == ".idea"
                or part == ".venv"
                or part.startswith("cmake-build-")
            ):
                return True
        return False

    files: list[Path] = []
    for r in roots:
        for p in r.rglob("*"):
            if _is_excluded(p):
                continue
            if p.is_file() and p.suffix in exts:
                files.append(p)
    return files


@arguably.command
def format(target: str | None = None, *, verbose: bool = False) -> None:
    """Format sources with clang-format

    :param target: vclient, vserver, vlib, or omit for all
    """
    clang = shutil.which("clang-format")
    if not clang:
        raise RuntimeError("clang-format not found. Please install clang-format.")
    files = _collect_files(target)
    if not files:
        print("[format] No files found")
        return
    print(f"[format] Formatting {len(files)} files...")
    for i, f in enumerate(files, 1):
        print(f"  [{i}/{len(files)}] {f}")
        env = os.environ.copy()
        if verbose:
            env["V_LOG_LEVEL"] = "trace"
        _run([clang, "-i", str(f)], env=env)


@arguably.command
def fmt(
    target: str | None = None, *, verbose: bool = False
) -> None:  # pragma: no cover - thin alias
    """Alias for format"""
    format(target, verbose=verbose)


def _find_test_exes(release: bool) -> list[Path]:
    bdir = _build_dir(release)
    tests_dir = bdir / "tests"
    if not tests_dir.exists():
        return []
    exes: list[Path] = []
    for p in tests_dir.iterdir():
        if p.is_file() and p.name.startswith(_exe_name("vtest_")):
            exes.append(p)
    return exes


@arguably.command
def test(*, release: bool = False, verbose: bool = False, full: bool = False) -> None:
    """Build (if needed) and run all tests under build/tests/ starting with vtest_

    :param release: Use Release build
    :param verbose: Pass --verbose to tests (enables TRACE output)
    :param full: Show full build output (not just fatal)
    """
    # Build everything to ensure tests are up-to-date
    build(target=None, release=release, verbose=verbose, full=full)
    # Discover tests
    exes = _find_test_exes(release)
    if not exes:
        raise RuntimeError("No test executables found under build/tests")
    print(f"[test] Running {len(exes)} tests...")
    failures = 0
    for exe in exes:
        print(f"[test] {exe.name}")
        try:
            cmd = [str(exe)]
            env = os.environ.copy()
            # Default tests omit debug & trace logs unless verbose
            if "V_LOG_LEVEL" not in env:
                env["V_LOG_LEVEL"] = "info"
            if verbose:
                env["V_LOG_LEVEL"] = "trace"
            _run(cmd, cwd=_build_dir(release), env=env)
        except subprocess.CalledProcessError as e:
            failures += 1
            print(f"[test] FAILED: {exe.name} (exit {e.returncode})")
    if failures:
        raise SystemExit(failures)
    print("[test] All tests passed")


def cli() -> int:  # pragma: no cover
    """Entry point for package runners (uvx, pipx, etc.). Returns exit code."""
    try:
        arguably.run()
        return 0
    except SystemExit as e:
        code = e.code if isinstance(e.code, int) else 1
        return code
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":  # pragma: no cover
    try:
        sys.exit(cli())
    except KeyboardInterrupt:
        print("Keyboard int received, stopping")
