# Slippage-Preserving Reshaping of Human-Made 3D Content
<img src="./page_assets/teaser.jpg" width="100%">

This repository contains the codebase for the paper titled "Slippage-Preserving Reshaping of Human-Made 3D Content" published in SIGGRAPH Asia 2023. For more details and to download the paper, please visit the [project website](https://www.cs.ubc.ca/labs/imager/tr/2023/3DReshaping/).

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
```
