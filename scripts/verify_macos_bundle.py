#!/usr/bin/env python3
import argparse
import plistlib
import re
import subprocess
import sys
from pathlib import Path


def parse_version(value):
    parts = [int(part) for part in value.split(".")]
    return tuple(parts + [0] * (3 - len(parts)))


def version_text(value):
    parts = list(value)
    while len(parts) > 1 and parts[-1] == 0:
        parts.pop()
    return ".".join(str(part) for part in parts)


def run(command):
    return subprocess.run(
        command,
        check=True,
        text=True,
        capture_output=True,
    ).stdout


def is_macho(path):
    description = run(["file", str(path)])
    return "Mach-O" in description


def collect_macho_files(app):
    macho_files = []
    for path in app.rglob("*"):
        if path.is_file() and not path.is_symlink() and is_macho(path):
            macho_files.append(path)
    return sorted(macho_files)


def collect_deployment_targets(path):
    output = run(["otool", "-l", str(path)])
    versions = []
    for block in re.split(r"(?=^Load command \d+$)", output, flags=re.MULTILINE):
        if re.search(r"^\s*cmd\s+LC_BUILD_VERSION$", block, re.MULTILINE):
            match = re.search(r"^\s*minos\s+([0-9]+(?:\.[0-9]+){1,2})", block, re.MULTILINE)
            if match:
                versions.append(parse_version(match.group(1)))
        elif re.search(r"^\s*cmd\s+LC_VERSION_MIN_MACOSX$", block, re.MULTILINE):
            match = re.search(r"^\s*version\s+([0-9]+(?:\.[0-9]+){1,2})", block, re.MULTILINE)
            if match:
                versions.append(parse_version(match.group(1)))
    return versions


def assert_contains_arch(path, expected_arch):
    archs = run(["lipo", "-archs", str(path)]).split()
    if expected_arch not in archs:
        raise AssertionError(f"{path} architectures {archs}, expected {expected_arch}")
    print(f"OK {path}: architectures {' '.join(archs)}")


def assert_linked(path, needle):
    linked = run(["otool", "-L", str(path)])
    if needle not in linked:
        raise AssertionError(f"{path} is not linked with {needle}")


def assert_not_linked(path, needle):
    linked = run(["otool", "-L", str(path)])
    if needle in linked:
        raise AssertionError(f"{path} must not link directly with {needle}")


def collect_linked_paths(path):
    output = run(["otool", "-L", str(path)])
    linked_paths = []
    for line in output.splitlines()[1:]:
        line = line.strip()
        if not line:
            continue
        linked_paths.append(line.split(" (", 1)[0])
    return linked_paths


def verify_runtime_linkage(executable, bundled_dylibs):
    bundled_dylib_names = {path.name for path in bundled_dylibs}
    failures = []

    for dylib in bundled_dylibs:
        linked_paths = collect_linked_paths(dylib)
        expected_install_name = f"@executable_path/{dylib.name}"
        if not linked_paths or linked_paths[0] != expected_install_name:
            actual_install_name = linked_paths[0] if linked_paths else "<missing>"
            failures.append(
                f"{dylib}: install name {actual_install_name!r}, expected {expected_install_name!r}"
            )

    for binary in [executable, *bundled_dylibs]:
        for linked_path in collect_linked_paths(binary):
            if linked_path.startswith("/"):
                if not (
                    linked_path.startswith("/System/Library/")
                    or linked_path.startswith("/usr/lib/")
                ):
                    failures.append(f"{binary}: absolute non-system dependency {linked_path}")
                continue

            dependency_name = Path(linked_path).name
            if dependency_name in bundled_dylib_names:
                expected_dependency = f"@executable_path/{dependency_name}"
                if linked_path != expected_dependency:
                    failures.append(
                        f"{binary}: bundled dependency {linked_path!r}, expected {expected_dependency!r}"
                    )

    if failures:
        lines = ["Invalid macOS runtime linkage:"]
        lines.extend(f"  {failure}" for failure in failures)
        raise AssertionError("\n".join(lines))

    print("OK macOS runtime linkage")


def verify_plist(app, minimum_macos):
    plist_path = app / "Contents" / "Info.plist"
    if not plist_path.is_file():
        raise AssertionError(f"Info.plist not found: {plist_path}")

    with plist_path.open("rb") as handle:
        plist = plistlib.load(handle)

    expected = {
        "LSUIElement": True,
        "CFBundleIdentifier": "com.momentics.GhostWireDesktop",
        "CFBundleExecutable": "GhostWireDesktop",
        "CFBundleIconFile": "GhostWire.icns",
        "LSMinimumSystemVersion": minimum_macos,
    }
    for key, value in expected.items():
        if plist.get(key) != value:
            raise AssertionError(f"Info.plist {key}={plist.get(key)!r}, expected {value!r}")
    print("OK Info.plist metadata")


def verify_deployment_targets(app, minimum):
    target = parse_version(minimum)
    failures = []
    macho_files = collect_macho_files(app)
    if not macho_files:
        raise AssertionError(f"No Mach-O files found in {app}")

    for path in macho_files:
        versions = collect_deployment_targets(path)
        if not versions:
            failures.append((path, "missing deployment target load command"))
            continue

        too_new = [version for version in versions if version > target]
        if too_new:
            failures.append((path, ", ".join(version_text(version) for version in too_new)))
        else:
            unique_versions = sorted(set(versions))
            print(f"OK {path}: min {', '.join(version_text(version) for version in unique_versions)}")

    if failures:
        lines = [f"Mach-O files requiring macOS newer than {minimum}:"]
        lines.extend(f"  {path}: {detail}" for path, detail in failures)
        raise AssertionError("\n".join(lines))


def verify_bundle(app, expected_arch, minimum_macos):
    contents = app / "Contents"
    executable = contents / "MacOS" / "GhostWireDesktop"
    ghostwire_dylib = contents / "MacOS" / "libghostwire.dylib"
    icon = contents / "Resources" / "GhostWire.icns"
    openssl_dylibs = sorted((contents / "MacOS").glob("libcrypto*.dylib"))
    openssl_dylibs += sorted((contents / "MacOS").glob("libssl*.dylib"))

    if not app.is_dir():
        raise AssertionError(f"App bundle not found: {app}")
    if not executable.is_file():
        raise AssertionError(f"Executable not found: {executable}")
    if not ghostwire_dylib.is_file():
        raise AssertionError(f"libghostwire.dylib not found: {ghostwire_dylib}")
    if not icon.is_file():
        raise AssertionError(f"App icon not found: {icon}")
    if len(openssl_dylibs) < 2:
        raise AssertionError(f"Expected bundled OpenSSL dylibs in {contents / 'MacOS'}")

    verify_plist(app, minimum_macos)
    assert_linked(executable, "AppKit.framework")
    assert_linked(executable, "UserNotifications.framework")
    assert_not_linked(executable, "libghostwire.dylib")

    bundled_dylibs = [ghostwire_dylib, *openssl_dylibs]
    verify_runtime_linkage(executable, bundled_dylibs)

    for binary in [executable, *bundled_dylibs]:
        assert_contains_arch(binary, expected_arch)

    verify_deployment_targets(app, minimum_macos)


def main():
    parser = argparse.ArgumentParser(description="Verify the GhostWire macOS app bundle.")
    parser.add_argument("--app", required=True, type=Path)
    parser.add_argument("--expected-arch", required=True)
    parser.add_argument("--minimum-macos", required=True)
    args = parser.parse_args()

    try:
        verify_bundle(args.app, args.expected_arch, args.minimum_macos)
    except (AssertionError, subprocess.CalledProcessError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
