#!/usr/bin/env python3
"""Novium's reproducible, isolated package manager.

Makes the Novium language better: easy C-ABI library dependency management,
 reproducible builds, and seamless integration with the Novium compiler.

Registry index format (JSON):
{"packages": {"demo": {"1.0.0": {"url": "file:///.../demo-1.0.0.zip",
"sha256": "...", "dependencies": {"core": "1.0.0"}, "c_dependencies": [{"include": "include", "library": "libfoo"}}]}}}
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile
import urllib.parse
import urllib.request
import zipfile
from pathlib import Path
from typing import Any

MANIFEST_NAME = "novium.json"
LOCK_NAME = "novium.lock.json"
STORE_NAME = ".novium/packages"
DEFAULT_REGISTRY = os.environ.get("NOVIUM_REGISTRY", "")


class PackageError(RuntimeError):
    pass


def project_root() -> Path:
    return Path.cwd()


def manifest_path(root: Path) -> Path:
    return root / MANIFEST_NAME


def load_json(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        raise PackageError(f"Missing {path.name}. Run `novium-pkg init` first.")
    except json.JSONDecodeError as error:
        raise PackageError(f"Invalid {path.name}: {error}") from error


def write_json_atomic(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=path.parent, delete=False) as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write("\n")
        temporary = Path(stream.name)
    temporary.replace(path)


def read_url(url: str) -> bytes:
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme in ("", "file"):
        path = Path(urllib.request.url2pathname(parsed.path if parsed.scheme else url))
        return path.read_bytes()
    if parsed.scheme != "https":
        raise PackageError("Only HTTPS and file:// package sources are allowed.")
    request = urllib.request.Request(url, headers={"User-Agent": "novium-pkg/0.2"})
    with urllib.request.urlopen(request, timeout=30) as response:
        return response.read()


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_registry(url: str) -> dict[str, Any]:
    if not url:
        raise PackageError("No registry configured. Set NOVIUM_REGISTRY to an HTTPS or file:// index URL.")
    try:
        index = json.loads(read_url(url).decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise PackageError(f"Registry index is not valid JSON: {error}") from error
    if not isinstance(index.get("packages"), dict):
        raise PackageError("Registry index must contain a 'packages' object.")
    return index


def exact_version(available: dict[str, Any], requirement: str) -> str:
    if not available:
        raise PackageError(
            f"No versions are available for this package in the registry."
        )
    if requirement in ("", "*", "latest"):  # latest = newest by semantic version
        return sorted(
            available.keys(),
            key=lambda value: tuple(int(x) if x.isdigit() else 0 for x in value.split(".")),
            reverse=True,
        )[0]
    if requirement.startswith("^"):  # caret: allow patch changes
        major = requirement[1:].split(".")[0]
        choices = [version for version in available if version.split(".")[0] == major]
        if choices:
            return sorted(choices, reverse=True)[0]
    if requirement in available:
        return requirement
    raise PackageError(
        f"No registry version satisfies '{requirement}'. Available: {', '.join(sorted(available))}"
    )


def safe_extract(archive: bytes, destination: Path) -> None:
    with tempfile.NamedTemporaryFile(suffix=".zip", delete=False) as stream:
        stream.write(archive)
        archive_path = Path(stream.name)
    try:
        with zipfile.ZipFile(archive_path) as package:
            root = destination.resolve()
            for member in package.infolist():
                target = (destination / member.filename).resolve()
                if not target.is_relative_to(root):
                    raise PackageError("Package archive contains an unsafe path.")
            package.extractall(destination)
    except zipfile.BadZipFile as error:
        raise PackageError("Package archive is not a valid ZIP file.") from error
    finally:
        archive_path.unlink(missing_ok=True)


def safe_join(base: Path, *parts: str) -> Path:
    """Join path parts under base, rejecting any traversal outside it."""
    root = base.resolve()
    candidate = root.joinpath(*parts).resolve()
    if not candidate.is_relative_to(root):
        raise PackageError(f"Unsafe path in package metadata: {Path(*parts)}")
    return candidate


def resolve_c_deps(
    c_deps: list[dict[str, str]], store: Path
) -> dict[str, dict[str, str]]:
    """Resolve C dependency include/library paths from the store.

    Each C dep dict can have:
      - "include": relative path from package root to headers
      - "library": relative path from package root to .lib / .so / .dylib
    Returns mapping of dep_name -> {"include": absolute_path, "library": absolute_path}
    """
    resolved: dict[str, dict[str, str]] = {}
    for dep in c_deps:
        inc_rel = dep.get("include", "")
        lib_rel = dep.get("library", "")
        version = str(dep.get("version", ""))
        inc_abs = safe_join(store, version, inc_rel) if inc_rel else Path()
        lib_abs = safe_join(store, version, lib_rel) if lib_rel else Path()
        # Also check common library extensions
        lib_found = False
        if lib_abs.exists():
            lib_found = True
        else:
            for ext in [".lib", ".a", ".so", ".dylib", ".dll"]:
                p = Path(str(lib_abs) + ext)
                if p.exists():
                    lib_abs = p
                    lib_found = True
                    break
        resolved[dep["name"]] = {"include": str(inc_abs), "library": str(lib_abs) if lib_found else ""}
    return resolved


def resolve(root: Path, registry_url: str) -> dict[str, dict[str, Any]]:
    manifest = load_json(manifest_path(root))
    registry = load_registry(registry_url)
    selected: dict[str, dict[str, Any]] = {}
    pending = list(manifest.get("dependencies", {}).items())
    while pending:
        name, requirement = pending.pop()
        versions = registry["packages"].get(name)
        if not isinstance(versions, dict):
            raise PackageError(f"Package '{name}' was not found in the registry.")
        version = exact_version(versions, str(requirement))
        current = selected.get(name)
        if current:
            if current["version"] != version:
                raise PackageError(
                    f"Dependency conflict for '{name}': {current['version']} vs {version}."
                )
            continue
        entry = versions[version]
        if not isinstance(entry, dict):
            raise PackageError(f"Registry metadata for {name}@{version} is incomplete.")
        if not isinstance(entry.get("url"), str) or not isinstance(entry.get("sha256"), str):
            raise PackageError(f"Registry metadata for {name}@{version} is incomplete.")
        selected[name] = {
            "version": version,
            "url": entry["url"],
            "sha256": entry["sha256"],
            "dependencies": entry.get("dependencies", {}),
            "c_dependencies": entry.get("c_dependencies", []),
        }
        pending.extend(entry.get("dependencies", {}).items())
    return selected


def setup_c_environment(root: Path, selected: dict[str, dict[str, Any]]) -> None:
    """Set up C dependency environment for Novium compilation.

    Adds C include paths and library paths to the Novium compiler's search paths.
    Writes a .novium-cdeps.json file that the Novium compiler can read.
    """
    store = root / STORE_NAME
    cdeps_file = root / ".novium-cdeps.json"
    c_include_dirs: list[str] = []
    c_library_dirs: list[str] = []
    c_libraries: list[str] = []

    for name, entry in selected.items():
        pkg_store = safe_join(store, name, entry["version"])
        for dep in entry.get("c_dependencies", []):
            inc_rel = dep.get("include", "")
            lib_rel = dep.get("library", "")
            # Include path
            if inc_rel:
                inc_path = safe_join(pkg_store, inc_rel)
                if inc_path.exists():
                    c_include_dirs.append(str(inc_path))
            # Library path
            if lib_rel:
                lib_path = safe_join(pkg_store, lib_rel)
                if lib_path.exists():
                    c_library_dirs.append(str(lib_path))
                    c_libraries.append(
                        os.path.basename(str(lib_path))
                        .replace(".lib", "")
                        .replace(".a", "")
                        .replace(".so", "")
                        .replace(".dylib", "")
                        .replace(".dll", "")
                    )

    cdeps_data = {
        "include_dirs": c_include_dirs,
        "library_dirs": c_library_dirs,
        "libraries": c_libraries,
    }
    cdeps_file.parent.mkdir(parents=True, exist_ok=True)
    write_json_atomic(cdeps_file, cdeps_data)


def install(root: Path, registry_url: str) -> None:
    selected = resolve(root, registry_url)
    store = root / STORE_NAME
    staging = Path(tempfile.mkdtemp(prefix="novium-pkg-", dir=root))
    try:
        replacement = staging / "packages"
        for name, entry in selected.items():
            payload = read_url(entry["url"])
            actual = sha256(payload)
            if actual.lower() != entry["sha256"].lower():
                raise PackageError(
                    f"Checksum mismatch for {name}@{entry['version']}; installation aborted."
                )
            target = safe_join(replacement, name, entry["version"])
            target.mkdir(parents=True, exist_ok=True)
            safe_extract(payload, target)
            write_json_atomic(target / ".novium-package.json", {"name": name, **entry})
        store.parent.mkdir(parents=True, exist_ok=True)
        old_store = store.with_name("packages.previous")
        if old_store.exists():
            shutil.rmtree(old_store)
        if store.exists():
            store.replace(old_store)
        replacement.replace(store)
        if old_store.exists():
            shutil.rmtree(old_store, ignore_errors=True)
        # Set up C dependency environment
        setup_c_environment(root, selected)
        write_json_atomic(root / LOCK_NAME, {"lockfileVersion": 1, "registry": registry_url, "packages": selected})
        print(f"Installed {len(selected)} package(s) into {store}.")
    finally:
        if staging.exists():
            shutil.rmtree(staging, ignore_errors=True)


def cmd_init(args: argparse.Namespace) -> None:
    path = manifest_path(project_root())
    if path.exists() and not args.force:
        raise PackageError(f"{MANIFEST_NAME} already exists (use --force to replace it).")
    write_json_atomic(path, {"name": args.name, "version": "0.1.0", "edition": "2026", "dependencies": {}, "c_dependencies": [], "build": {"target": "native", "profile": "debug"}})
    print(f"Created {path}")


def cmd_add(args: argparse.Namespace) -> None:
    root = project_root()
    manifest = load_json(manifest_path(root))

    # Split off any C dependency settings: name[@version][,c_include=...,c_library=...]
    raw = args.package
    c_part = ""
    if "," in raw:
        raw, c_part = raw.split(",", 1)
        c_part = c_part.strip()

    name, separator, requirement = raw.partition("@")
    if not name:
        raise PackageError("Package name cannot be empty.")

    manifest.setdefault("dependencies", {})[name] = requirement if separator else "*"
    if c_part:
        # Keep the raw C dependency settings; they are resolved during install
        # by setup_c_environment using the registry metadata.
        manifest.setdefault("c_dependencies", {})[name] = c_part

    write_json_atomic(manifest_path(root), manifest)
    install(root, args.registry)


def cmd_remove(args: argparse.Namespace) -> None:
    root = project_root()
    manifest = load_json(manifest_path(root))
    if args.package not in manifest.get("dependencies", {}):
        raise PackageError(f"'{args.package}' is not a direct dependency.")
    del manifest["dependencies"][args.package]
    # Also remove C deps referencing this package
    cdeps_file = root / ".novium-cdeps.json"
    if cdeps_file.exists():
        cdeps = load_json(cdeps_file)
        # Remove entries referencing this package
        cdeps["library_dirs"] = [d for d in cdeps.get("library_dirs", []) if args.package not in d]
        cdeps["include_dirs"] = [d for d in cdeps.get("include_dirs", []) if args.package not in d]
        write_json_atomic(cdeps_file, cdeps)
    write_json_atomic(manifest_path(root), manifest)
    install(root, args.registry)


def cmd_list(_: argparse.Namespace) -> None:
    lock = load_json(project_root() / LOCK_NAME)
    for name, entry in sorted(lock.get("packages", {}).items()):
        print(f"{name} {entry['version']}")


def cmd_sync(args: argparse.Namespace) -> None:
    install(project_root(), args.registry)


def main() -> int:
    parser = argparse.ArgumentParser(prog="novium-pkg", description="Novium package manager - reproducible, isolated dependency management")
    parser.add_argument("--registry", default=DEFAULT_REGISTRY, help="HTTPS or file:// registry index URL")
    commands = parser.add_subparsers(dest="command", required=True)

    init = commands.add_parser("init")
    init.add_argument("name", nargs="?", default="my-novium-app")
    init.add_argument("--force", action="store_true")
    init.set_defaults(handler=cmd_init)

    add = commands.add_parser("add")
    add.add_argument("package", help="Package name or name@version (e.g., core@1.2.3)")
    add.set_defaults(handler=cmd_add)

    remove = commands.add_parser("remove")
    remove.add_argument("package", help="Package name to remove")
    remove.set_defaults(handler=cmd_remove)

    sync = commands.add_parser("sync")
    sync.add_argument("--registry", default=DEFAULT_REGISTRY, help="Registry URL override")
    sync.set_defaults(handler=cmd_sync)

    listing = commands.add_parser("list")
    listing.set_defaults(handler=cmd_list)

    args = parser.parse_args()
    try:
        args.handler(args)
        return 0
    except PackageError as error:
        print(f"novium-pkg: error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())