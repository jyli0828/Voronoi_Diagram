"""Generate the merged source file (程式原始碼合併檔) M143040074.txt.

Usage (from the project folder):
    python tools/merge_sources.py

All headers in include/ and all sources in src/ are copied verbatim, headers first,
each preceded by a separator with its file name. Edit the header lines below if the
build environment changes.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "M143040074.txt"

HEADER_INFO = [
    "程式原始碼合併檔 (Merged source code) - Voronoi Diagram",
    "Author: 李俊諺 Jun-Yan Li",
    "Student ID: M143040074",
    "Language: C++ (C++17)",
    "Build environment: Qt 6.10.1 (Qt Widgets), MinGW-w64 GCC 13.1.0 (64-bit), qmake",
    "                   Project file: VoronoiDiagram.pro",
    "OS: Windows 11",
]

NEWLINE = "\r\n"
BAR = "// " + "=" * 77


def source_files():
    by_name = lambda p: p.name.lower()
    headers = sorted((ROOT / "include").glob("*.h"), key=by_name)
    sources = sorted((ROOT / "src").glob("*.cpp"), key=by_name)
    return headers + sources


def main():
    files = source_files()
    names = [f.relative_to(ROOT).as_posix() for f in files]

    # First line must be the language tag required by the course
    header = ["// $LAN=C++$", BAR]
    header += ["// " + line for line in HEADER_INFO]
    header += ["// Files:"] + [f"//   {i}. {name}" for i, name in enumerate(names, 1)]
    header += [BAR, ""]
    text = NEWLINE.join(header)

    for i, (path, name) in enumerate(zip(files, names), 1):
        content = path.read_bytes().decode("utf-8")
        banner = ["", "", BAR, f"// File {i}/{len(files)}: {name}", BAR, ""]
        text += NEWLINE.join(banner) + content

    OUTPUT.write_bytes(text.encode("utf-8"))
    print(f"Wrote {OUTPUT.name}: {len(files)} files")


if __name__ == "__main__":
    main()
