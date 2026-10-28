#!/usr/bin/env python3
"""Generate the Docker configuration (ilvo/data) from the package defaults.

The package defaults are the single source of truth. ilvo/data is a copy of
them with the Docker-specific values of ilvo/docker-overrides.json applied:

    scripts/sync-configs.py          regenerate ilvo/data
    scripts/sync-configs.py --check  exit 1 when ilvo/data is out of date (CI)

Overrides are merged into the defaults: objects are merged key by key, other
values (lists included) are replaced, and null removes a key. A list of
objects with a "Name" (e.g. ilvoAddons) is patched per name.
"""
import filecmp
import json
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULTS = ROOT / "artof-core/usr/share/artof-core/defaults"
OVERRIDES = ROOT / "ilvo/docker-overrides.json"
TARGET = ROOT / "ilvo/data"
# Generated content of ilvo/data; anything else there (logs, node-red) is left alone
MANAGED = ["config.json", "settings.json", "types.json", "redis.init.json", "field", "implement"]


def merge(base, patch):
    if isinstance(patch, dict) and isinstance(base, list):
        merged = []
        for item in base:
            name = item.get("Name") if isinstance(item, dict) else None
            merged.append(merge(item, patch[name]) if name in patch else item)
        unknown = set(patch) - {item.get("Name") for item in base if isinstance(item, dict)}
        if unknown:
            sys.exit(f"Override for unknown entries: {', '.join(sorted(unknown))}")
        return merged
    if isinstance(patch, dict) and isinstance(base, dict):
        merged = dict(base)
        for key, value in patch.items():
            if value is None:
                merged.pop(key, None)
            else:
                merged[key] = merge(base.get(key), value)
        return merged
    return patch


def generate(out):
    overrides = json.loads(OVERRIDES.read_text())
    for name in MANAGED:
        src = DEFAULTS / name
        if src.is_dir():
            shutil.copytree(src, out / name)
        elif name in overrides:
            data = merge(json.loads(src.read_text()), overrides[name])
            (out / name).write_text(json.dumps(data, indent=4, ensure_ascii=False) + "\n")
        else:
            shutil.copy2(src, out / name)
    unused = set(overrides) - set(MANAGED)
    if unused:
        sys.exit(f"Overrides for files that are not generated: {', '.join(sorted(unused))}")


def differences(expected, actual):
    """Paths below the managed entries that differ between two folders."""
    diffs = []

    def walk(cmp, prefix):
        diffs.extend(prefix + n for n in cmp.left_only + cmp.right_only + cmp.diff_files + cmp.funny_files)
        for sub, subcmp in cmp.subdirs.items():
            walk(subcmp, f"{prefix}{sub}/")

    for name in MANAGED:
        e, a = expected / name, actual / name
        if e.is_dir() and a.is_dir():
            walk(filecmp.dircmp(e, a, ignore=[]), f"{name}/")
        elif not (e.is_file() and a.is_file() and filecmp.cmp(e, a, shallow=False)):
            diffs.append(name)
    return diffs


def main():
    check = "--check" in sys.argv[1:]
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp)
        generate(out)
        diffs = differences(out, TARGET)
        if check:
            if diffs:
                print(f"{TARGET.relative_to(ROOT)} is out of date with the package defaults:")
                print("\n".join(f"  {d}" for d in diffs))
                print("Run scripts/sync-configs.py and commit the result.")
                sys.exit(1)
            print(f"{TARGET.relative_to(ROOT)} is in sync with the package defaults.")
            return
        TARGET.mkdir(parents=True, exist_ok=True)
        for name in MANAGED:
            dest = TARGET / name
            if dest.is_dir():
                shutil.rmtree(dest)
            elif dest.exists():
                dest.unlink()
            shutil.move(str(out / name), dest)
        print("\n".join(f"updated {d}" for d in diffs) or "Nothing to update.")


if __name__ == "__main__":
    main()
