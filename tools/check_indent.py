#!/usr/bin/env python3
"""
check_indent.py — Flag files that mix tabs and spaces for indentation.

Usage:
    python check_indent.py <path> [<path> ...]
    python check_indent.py src/

Exit code:
    0 = clean
    1 = violations found
    2 = usage error
"""

import sys
from pathlib import Path

# Extensions we care about. Add/remove as needed.
EXTENSIONS = {".c", ".cpp", ".h", ".hpp", ".cc"}

# Files to skip entirely.
SKIP_DIRS = {".git", "build", ".pio", ".vscode", "node_modules", "managed_components"}


def check_file(path: Path) -> list[tuple[int, str, str]]:
    """
    Return a list of (line_number, reason, snippet) tuples for a single file.
    """
    violations = []
    try:
        with path.open("r", encoding="utf-8", errors="replace") as f:
            for lineno, raw in enumerate(f, start=1):
                line = raw.rstrip("\n\r")
                if not line.strip():
                    continue  # blank line, skip

                leading = line[: len(line) - len(line.lstrip("\t "))]
                has_tab = "\t" in leading
                has_space = " " in leading

                if has_tab and has_space:
                    violations.append(
                        (lineno, "MIXED tabs and spaces", leading.replace("\t", "→   ").replace(" ", "·") + "…")
                    )
                elif has_tab:
                    violations.append(
                        (lineno, "TAB indentation", leading.replace("\t", "→") + "…")
                    )
    except (OSError, UnicodeDecodeError) as e:
        violations.append((0, f"READ ERROR: {e}", ""))
    return violations


def iter_targets(paths: list[str]) -> list[Path]:
    """Expand directories into a flat list of source files."""
    targets = []
    for p in paths:
        root = Path(p)
        if root.is_file():
            targets.append(root)
        elif root.is_dir():
            for child in root.rglob("*"):
                if not child.is_file():
                    continue
                if child.suffix not in EXTENSIONS:
                    continue
                if any(part in SKIP_DIRS for part in child.parts):
                    continue
                targets.append(child)
        else:
            print(f"[WARN] Path not found: {p}", file=sys.stderr)
    return targets


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2

    targets = iter_targets(argv[1:])
    if not targets:
        print("[INFO] No source files found.")
        return 0

    total_violations = 0
    files_with_violations = 0

    for path in sorted(targets):
        violations = check_file(path)
        if not violations:
            continue
        files_with_violations += 1
        print(f"\n=== {path} ===")
        for lineno, reason, snippet in violations:
            total_violations += 1
            if lineno:
                print(f"  {path}:{lineno}: {reason}")
                print(f"      {snippet}")
            else:
                print(f"  {path}: {reason}")

    print()
    print(f"Scanned {len(targets)} files.")
    print(f"Violations: {total_violations} in {files_with_violations} file(s).")
    return 1 if total_violations else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))