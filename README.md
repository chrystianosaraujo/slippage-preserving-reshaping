# Slippage-Preserving Reshaping of Human-Made 3D Content
<img src="./page_assets/teaser.jpg" width="100%">

This repository contains the codebase for the paper titled "Slippage-Preserving Reshaping of Human-Made 3D Content" published in SIGGRAPH Asia 2023. Please visit the [project website](https://www.cs.ubc.ca/labs/imager/tr/2023/3DReshaping/) for more details and download the paper.

**Slippage-Preserving Reshaping of Human-Made 3D Content**<br>
[Chrystiano Araújo](https://www.cs.ubc.ca/~araujoc/), [Nicholas Vining](https://www.cs.ubc.ca/~nvining/), [Silver Burla](https://silverburla.com/), [Manuel Ruivo de Oliveira](), [Enrique Rosales](https://www.enrique-rosales.com/), and [Alla Sheffer](http://www.cs.ubc.ca/~sheffa/) <br>
ACM Transaction on Graphics (Proceedings of SIGGRAPH Asia 2023) <br>

## Installation
Clone this repository to get started.
```
git clone https://github.com/chrystianosaraujo/slippage-preserving-reshaping.git
```

Only Windows build is supported at the moment (specifically tested on Visual Studio 2022 [v143]). Stay tuned for updates as we add support to other platforms.

Besides the MSVC compiler, the only dependency is cmake, as our build system automatically downloads all other dependencies.

## Compilation
Use the following instructions to build in release mode.

```
cd [PROJECT_ROOT]
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

Now, either open the VS project or build it directly from command line as follows:

```
cmake --build . --config Release
```

## Project Structure
```
README.md
include
source
apps
    reshaping_app
    reshaping_demo
models
```
+ The reshaping library codebase can be found in the `./include` and `./source` folders.
+ The `./apps/reshaping_demo` directory contains a demo example demonstrating how to use the reshaping library.
+ The `./apps/reshaping_app` directory contains a prototype for a GUI-based app that utilizes the reshaping library. See the `Usage` section for details on how to use it.
+ The `./apps/models` directory contains input examples for reshaping tasks. Each obj file is accompanied by the following files:
    +  `.fk`: precomputed per-face curvatures
    +  `.deform`: pre-defined edit operations. Each operation is identified by a unique label.
    +  `.cam`: pre-defined camera settings for each edit operation label defined in the `.deform` file
    +  `.straight`: optional file containing straightness information

After building the project, the reshaping lib, and reshaping_app and reshaping_demo executables can be found as follows:
+ `mesh_reshaping_lib.lib` can be found under `./build/Release`
+ `reshaping_demo.exe` can be found under `./build/apps/reshaping_demo/Release/`
+ `reshaping_app.exe` can be found under `./build/apps/reshaping_app/Release/`

## Usage

### mesh_reshaping.lib
Check the `./apps/reshaping_demo` folder for a demo example of how to use the reshaping library.

### reshaping_demo
To run the reshaping demo, use the following command:
```bash
reshaping_demo.exe -i <input_fn> -o <output_dir> -e <edit_operation_label>
```

where:
+ `<input_fn>`: input mesh filename (obj)
+ `<output_dir>`: directory where all output files will be saved. 
+ `<edit_operation_label>`: label identifying an edit operation that must be loaded from the model `.deform` file. If no label is provided (no `-e` argument used), all the available edit operations will be listed for further use.

Once the reshaping process is finished, the output folder will contain obj files for input (`_input.obj`), displaced points (`_handles.obj`), fixed points (`_fixed_points.obj`), and the final reshape surface (`_output.obj`).

### reshaping_app
The GUI-based reshaping app offers the following functionalities:

* provides a **yet-rudimentary** UI for creating, editing, and deleting _editing operations_ for an input mesh
* save and load pre-defined cameras for _editing operations_ already created
* provides a few visualization options for the surface mesh and _editing operation_
* screenshot options
* most of the rendering and running options are also directly available through command-line execution.


To run the reshaping app, use the following command:
```bash
reshaping_app.exe -o <output_dir>
```

Use `--help` for a list of all available command line options.

<p align="center">
    <img src="./page_assets/reshaping_app.jpg" width="65%">
</p>

Below are the details for each numbered functionality highlighted in the image above:

1. Camera manipulation
2. Vertex selection
3. Handle annotation. Each clicked vertex is defined as a handle that can be further displaced.
4. Fixed point annotation. Each clicked vertex is defined as a fixed point.
5. Erases handle or fixed point annotations
6. Translates previously annotated handles. In the current version, all handles are transformed at once.
7. Rotates previously annotated handles. In the current version, all handles are transformed at once.
8. Scales previously annotated handles. In the current version, all handles are transformed at once.
9. Runs the `reshaping` tool. Note that this must be called after selecting a pre-defined _editing operation_.
10. List of all pre-defined _editing operation_ available for use.
11. Saves the current _edit operation_ by providing a label and clicking on the "Save" button.
12. List of all pre-defined _camera_ available for use. The current camera can also be saved similarly to the item above.
13. Visualization options available for surface mesh and _editing operation_

**Follow the steps below for quick use:**
+ Load mesh using menu File->"Load Mesh"
+ Activate an  _"edit operation"_ by selecting one in the list of available operations (#10 in the image above)
+ Press "Run Reshaping" button (#9 in the image above)
 







