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
        # Only main targets have individual presets
        if target in ["vclient", "vserver", "vlib"]:
            return f"{base}-{target}"
        else:
            # Test targets use general build preset
            return f"{base}-build"
    return f"{base}-build"


def _build_dir(release: bool) -> Path:
    return BUILD_DIR_RELEASE if release else BUILD_DIR_DEBUG


def _run(
    cmd: list[str], cwd: Path | None = None, env: dict[str, str] | None = None
) -> None:
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, env=env, check=True)


def _detect_cmake_targets(release: bool = False) -> dict[str, str]:
    """Detect available CMake targets and classify them.

    Returns: Dict mapping target_name -> target_type ('exe', 'lib', 'test', 'utility')
    """
    bdir = _build_dir(release)
    targets = {}

    # Always include main project targets (known to exist from CMakeLists.txt)
    targets.update(_discover_targets_static())

    # Try ninja targets if build is configured for additional discovered targets
    if (bdir / "build.ninja").exists():
        try:
            result = subprocess.run(
                ["ninja", "-t", "targets"],
                cwd=bdir,
                capture_output=True,
                text=True,
                check=True
            )
            # Filter for additional project-specific targets
            for line in result.stdout.splitlines():
                target = line.split(":")[0].strip()
                if _is_project_target(target) and target not in targets:
                    targets[target] = _classify_target(target)
        except (subprocess.CalledProcessError, FileNotFoundError):
            pass

    return targets


def _is_project_target(target: str) -> bool:
    """Check if a target belongs to this project (not external deps)."""
    # Include main project targets
    if target in ["vclient", "vserver", "vlib"]:
        return True
    # Include test targets
    if target.startswith("vtest_"):
        return True
    # Exclude cp_resources - it's just a utility for copying files
    # Include experiment targets (future)
    if target.startswith("vexp_"):
        return True

    # Exclude all external dependency and CMake utility targets
    excluded_prefixes = [
        "absl_", "SDL", "EnTT", "spdlog", "Taskflow", "imgui", "daxa",
        "extern/", "package", "install", "edit_cache", "rebuild_cache",
        "lib", "Continuous", "Experimental", "Nightly", "build.ninja",
        "clean", "help", "list_install_components", "glm", "reflectcpp", "ser20",
        "cp_resources"  # Exclude copy utility
    ]
    if any(target.startswith(prefix) for prefix in excluded_prefixes):
        return False

    # Exclude targets that look like CMake utilities
    if target in ["clean", "help", "build.ninja", "package", "install", "cp_resources"]:
        return False

    # Only include our specific known targets to be safe
    return False


def _classify_target(target: str) -> str:
    """Classify a target by type."""
    if target.startswith("vtest_"):
        return "test"
    elif target in ["vclient", "vserver"]:
        return "exe"
    elif target in ["vlib"]:
        return "lib"
    else:
        # Try to infer from name
        if "test" in target.lower():
            return "test"
        elif target.endswith("lib") or "lib" in target:
            return "lib"
        else:
            return "exe"  # Assume executable by default


def _discover_targets_static() -> dict[str, str]:
    """Fallback target discovery by scanning project structure."""
    targets = {}

    # Main executables (from CMakeLists.txt analysis)
    targets["vclient"] = "exe"
    targets["vserver"] = "exe"
    targets["vlib"] = "lib"

    # Discover test targets
    tests_dir = ROOT / "tests"
    if tests_dir.exists():
        for test_dir in tests_dir.iterdir():
            if test_dir.is_dir() and (test_dir / "main.cpp").exists():
                targets[f"vtest_{test_dir.name}"] = "test"

    # Discover future experiment targets
    experiments_dir = ROOT / "experiments"
    if experiments_dir.exists():
        for exp_dir in experiments_dir.iterdir():
            if exp_dir.is_dir() and (exp_dir / "main.cpp").exists():
                targets[f"vexp_{exp_dir.name}"] = "exe"

    return targets


def _is_executable_target(target: str, target_type: str, release: bool = False) -> bool:
    """Check if a target produces an executable that can be run."""
    if target_type in ["lib", "utility"]:
        return False

    # Check if executable exists after building
    bdir = _build_dir(release)
    if target_type == "test":
        exe_path = bdir / "tests" / _exe_name(target)
    else:
        exe_path = bdir / _exe_name(target)
    return exe_path.exists() and exe_path.is_file()


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

    :param target: Optional target name. If omitted, builds all. Use 'targets' command to list available targets
    :param release: Build in Release mode (default Debug)
    :param full: Show full build output (not just fatal)
    """
    _ensure_configured(release)
    preset = _build_preset(target, release)
    cmd = ["cmake", "--build", "--preset", preset]

    # For test targets, add --target to specify the specific target
    if target and target not in ["vclient", "vserver", "vlib"]:
        cmd.extend(["--target", target])

    print(f"[build] {' '.join(cmd)}")
    print("Starting build")
    env = os.environ.copy()
    if verbose:
        env["V_LOG_LEVEL"] = "trace"

    if full:
        _run(cmd, env=env)
        return

    import subprocess as _sp

    proc = _sp.Popen(
        cmd,
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
    """Run any executable target

    :param target: Target name (vclient, vserver, vtest_domain, etc.)
    :param release: Use Release build dir
    :param full: Show full build output (not just fatal)
    """
    # Detect all targets and find the requested one
    targets = _detect_cmake_targets(release)

    if target not in targets:
        available = [name for name, type_ in targets.items() if type_ in ["exe", "test"]]
        raise ValueError(f"Unknown target '{target}'. Available executable targets: {', '.join(available)}")

    target_type = targets[target]
    if not _is_executable_target(target, target_type, release):
        raise ValueError(f"Target '{target}' is not executable (type: {target_type})")

    # Build the target
    build(target=target, release=release, full=full, verbose=verbose)

    # Determine executable path based on target type
    if target_type == "test":
        exe = _build_dir(release) / "tests" / _exe_name(target)
    else:
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


@arguably.command
def targets(*, release: bool = False) -> None:
    """List all available CMake targets in this project

    :param release: Use Release build configuration for detection
    """
    print(f"[targets] Discovering targets using {'Release' if release else 'Debug'} configuration...")

    discovered_targets = _detect_cmake_targets(release)

    if not discovered_targets:
        print("[targets] No targets found")
        return

    print(f"[targets] Found {len(discovered_targets)} targets:")
    print()

    # Group by type for better display
    by_type = {}
    for target, target_type in discovered_targets.items():
        if target_type not in by_type:
            by_type[target_type] = []
        by_type[target_type].append(target)

    # Display in logical order
    type_order = ["exe", "lib", "test", "utility"]
    type_names = {
        "exe": "Executables",
        "lib": "Libraries",
        "test": "Tests",
        "utility": "Utilities"
    }

    for target_type in type_order:
        if target_type in by_type:
            targets_list = sorted(by_type[target_type])
            print(f"  {type_names[target_type]}:")
            for target in targets_list:
                # Target names already have v prefix, so use them directly
                print(f"    {target:<20} (CMake target)")
            print()

    print("Usage examples:")
    print("  ./v.py vclient --run-after    # Build and run vclient")
    print("  ./v.py vtest_domain           # Build vtest_domain")
    print("  ./v.py vtest_domain --run-after  # Build and run vtest_domain")
    print("  ./v.py vlib                   # Build vlib (library)")
    print()
    print("All targets support --release, --verbose, and --full flags")


@arguably.command
def list_targets(*, release: bool = False) -> None:
    """Alias for targets command"""
    targets(release=release)


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
        if p.is_file() and p.name.startswith("vtest_") and p.name == _exe_name(p.stem):
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
        tests_dir = _build_dir(release) / "tests"
        raise RuntimeError(f"No test executables found under {tests_dir}")
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




# Just define the target commands statically - simpler and works
@arguably.command
def vclient(
    *,
    release: bool = False,
    verbose: bool = False,
    full: bool = False,
    run_after: bool = False,
) -> None:
    """Build vclient executable

    :param release: Build in Release mode (default Debug)
    :param verbose: Enable verbose logging
    :param full: Show full build output (not just fatal)
    :param run_after: Run the target after building
    """
    build(target="vclient", release=release, verbose=verbose, full=full)
    if run_after:
        exe = _build_dir(release) / _exe_name("vclient")
        if exe.exists():
            print(f"[run] {exe}")
            env = os.environ.copy()
            if verbose:
                env["V_LOG_LEVEL"] = "trace"
            _run([str(exe)], cwd=_build_dir(release), env=env)
        else:
            print(f"[run] Executable not found: {exe}")


@arguably.command
def vserver(
    *,
    release: bool = False,
    verbose: bool = False,
    full: bool = False,
    run_after: bool = False,
) -> None:
    """Build vserver executable

    :param release: Build in Release mode (default Debug)
    :param verbose: Enable verbose logging
    :param full: Show full build output (not just fatal)
    :param run_after: Run the target after building
    """
    build(target="vserver", release=release, verbose=verbose, full=full)
    if run_after:
        exe = _build_dir(release) / _exe_name("vserver")
        if exe.exists():
            print(f"[run] {exe}")
            env = os.environ.copy()
            if verbose:
                env["V_LOG_LEVEL"] = "trace"
            _run([str(exe)], cwd=_build_dir(release), env=env)
        else:
            print(f"[run] Executable not found: {exe}")


@arguably.command
def vlib(
    *,
    release: bool = False,
    verbose: bool = False,
    full: bool = False,
    run_after: bool = False,
) -> None:
    """Build vlib library

    :param release: Build in Release mode (default Debug)
    :param verbose: Enable verbose logging
    :param full: Show full build output (not just fatal)
    :param run_after: Run the target after building (ignored for libraries)
    """
    build(target="vlib", release=release, verbose=verbose, full=full)
    if run_after:
        print("[run] Target 'vlib' is not executable (type: library)")


@arguably.command
def vtest_domain(
    *,
    release: bool = False,
    verbose: bool = False,
    full: bool = False,
    run_after: bool = False,
) -> None:
    """Build vtest_domain test

    :param release: Build in Release mode (default Debug)
    :param verbose: Enable verbose logging
    :param full: Show full build output (not just fatal)
    :param run_after: Run the test after building
    """
    build(target="vtest_domain", release=release, verbose=verbose, full=full)
    if run_after:
        exe = _build_dir(release) / "tests" / _exe_name("vtest_domain")
        if exe.exists():
            print(f"[run] {exe}")
            env = os.environ.copy()
            if verbose:
                env["V_LOG_LEVEL"] = "trace"
            _run([str(exe)], cwd=_build_dir(release), env=env)
        else:
            print(f"[run] Test executable not found: {exe}")


@arguably.command
def vtest_net(
    *,
    release: bool = False,
    verbose: bool = False,
    full: bool = False,
    run_after: bool = False,
) -> None:
    """Build vtest_net test

    :param release: Build in Release mode (default Debug)
    :param verbose: Enable verbose logging
    :param full: Show full build output (not just fatal)
    :param run_after: Run the test after building
    """
    build(target="vtest_net", release=release, verbose=verbose, full=full)
    if run_after:
        exe = _build_dir(release) / "tests" / _exe_name("vtest_net")
        if exe.exists():
            print(f"[run] {exe}")
            env = os.environ.copy()
            if verbose:
                env["V_LOG_LEVEL"] = "trace"
            _run([str(exe)], cwd=_build_dir(release), env=env)
        else:
            print(f"[run] Test executable not found: {exe}")


@arguably.command
def vtest_svo(
    *,
    release: bool = False,
    verbose: bool = False,
    full: bool = False,
    run_after: bool = False,
) -> None:
    """Build vtest_svo test

    :param release: Build in Release mode (default Debug)
    :param verbose: Enable verbose logging
    :param full: Show full build output (not just fatal)
    :param run_after: Run the test after building
    """
    build(target="vtest_svo", release=release, verbose=verbose, full=full)
    if run_after:
        exe = _build_dir(release) / "tests" / _exe_name("vtest_svo")
        if exe.exists():
            print(f"[run] {exe}")
            env = os.environ.copy()
            if verbose:
                env["V_LOG_LEVEL"] = "trace"
            _run([str(exe)], cwd=_build_dir(release), env=env)
        else:
            print(f"[run] Test executable not found: {exe}")




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
