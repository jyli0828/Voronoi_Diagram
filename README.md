# Voronoi Diagram

Term project for Design and Analysis of Algorithms: a program that draws Voronoi diagrams with the divide-and-conquer algorithm from the textbook (C++, Qt).

Jun-Yan Li (M143040074), first-year M.S. student, Computer Science and Engineering  
Requirements: <a href="https://nsysucbyang.github.io/my-site/course/algo/algo_report.htm" target="_blank">Algorithms Term Project guidelines</a> (in Chinese)  
Grading criteria: <a href="https://nsysucbyang.github.io/my-site/course/algo/Voronoi_evaluate_first.pdf" target="_blank">first evaluation</a>, <a href="https://nsysucbyang.github.io/my-site/course/algo/Voronoi_evaluate_second.pdf" target="_blank">second evaluation</a> (in Chinese)  
Full report: <a href="https://jyli0828.github.io/voronoi-diagram/M143040074.html" target="_blank">M143040074.html</a> (in Chinese)

<a href="img/UI.png" target="_blank"><img src="img/UI.png" alt="Program window"></a>

## Features

- Add points by clicking, by typing (X, Y), or from an input file; an output file can also be read to show its diagram directly.
- When an input file has several test cases, the first one is drawn, and the **Next test case** button on the toolbar steps through the rest.
- **Run** draws the result at once; **Step** splits each merge into 7 steps (the Voronoi diagrams and convex hulls of the left and right halves, the hyperplane, and trimming), drawn one at a time in different colors, with the current step shown in the status bar.
- Save the result as an output text file.

## Running

Download `VoronoiDiagram-win64.7z` from <a href="https://github.com/jyli0828/voronoi-diagram/releases/latest" target="_blank">Releases</a>, extract it and run `VoronoiDiagram/VoronoiDiagram.exe`; no installation is needed. Keep the executable together with the DLLs and the `platforms/` and `styles/` folders next to it.

## Building

Requires Qt 6.10 (MinGW 64-bit). Open `VoronoiDiagram.pro` in Qt Creator and build it, or run the following in cmd (change the Qt paths to match your installation):

```
set PATH=C:\Qt\6.10.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%
mkdir build-release
cd build-release
qmake ..\VoronoiDiagram.pro CONFIG+=release
mingw32-make
```

Then, in the same cmd window, package it into `exe/` with only the DLLs and plugins it needs, the same contents as the archive in Releases:

```
mkdir ..\exe
copy release\VoronoiDiagram.exe ..\exe\
windeployqt --release --no-translations --no-opengl-sw --no-system-d3d-compiler --skip-plugin-types generic,iconengines,imageformats,networkinformation,tls ..\exe\VoronoiDiagram.exe
```

## File formats

- **Input file**: each test case starts with a line holding the number of points n, followed by n lines of `x y`. A count of 0 ends the file, and text after `#` is a comment.
- **Output file**: `P x y` (points) and `E x1 y1 x2 y2` (edges) with integer coordinates, sorted in lexical order; rays are clipped to the canvas border (610 × 610).

## Project structure

| Path | Contents |
|---|---|
| `src/`, `include/` | Source code; the algorithm is in `VoronoiDiagram.cpp` |
| `VoronoiDiagram.pro` | qmake project file |
| `exe/` | Packaged executable and the DLLs it needs (created by the packaging commands above, not tracked by git) |
| `testcase/input/` | Test input files |
| `testcase/output/` | Output file of each test case |
| `testcase/sample_output.txt` | Sample of the output format (11 × 11 grid) |
| `img/` | Images for the report |
| `M143040074.html` | Report |
| `M143040074.txt` | All source files merged into one file |
| `tools/merge_sources.py` | Script that creates the merged source file |
| `tools/pointGenerator.py` | Generates random test points |
| `tools/imageCheck/` | Result image checker: checks the drawn diagrams pixel by pixel against brute force |
| `LICENSE` | MIT License |

## Creating the merged source file

After changing the source code, run in the project folder:

```
python tools/merge_sources.py
```

## Checking the result images

`tools/imageCheck` draws every test case of an input file on the program's canvas (the same as pressing Run) and compares it pixel by pixel with a brute-force answer. The answer does not use `VoronoiDiagram.cpp`: the perpendicular bisector of each pair of points, cut short by the other points, gives the Voronoi edges, and the convex hull is found separately with monotone chain. It checks:

- Every Voronoi edge is drawn as a red line, and there are no extra red pixels (within 2 px).
- The cyan convex hull lines, in the same way.
- Every point has a black dot, and there are no extra black pixels.

Build and run it in cmd in the project folder (with PATH set as in Building):

```
mkdir build-imageCheck
cd build-imageCheck
qmake ..\tools\imageCheck\imageCheck.pro
mingw32-make
release\imageCheck.exe "..\testcase\input\52,77,92,150,200.txt" --save .
```

It prints one line per test case and ends with `0 of 5 images wrong` when all of them are correct. Other options:

- `--save <folder>`: also save the drawn images as `<input file name>_<k>.png` (the folder must exist; the example above saves them in `build-imageCheck`).
- `--images <file name pattern>`: check existing PNG files instead, with `%1` standing for the test case number, for example `--images "52,77,92,150,200_%1.png"`.
- `--mutate`: damage each image on purpose first (erase part of an edge, add a fake line, move an edge by 3 px, erase a point) to confirm that the check catches mistakes; every test case should then be `WRONG`.
- `--where`: print the position of each mistake.

## License

[MIT License](LICENSE)
