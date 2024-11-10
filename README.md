# PCB-BVH-UI: Interactive 2D PCB Visualization and Analysis Tool

![teaser](.\assets\teaser.png)

[**[Video]**]([(1) PCB-BVH for Collision Detection and Closest Point Query - YouTube](https://www.youtube.com/watch?v=AMjZWk77mFc))

Welcome to PCB-BVH-UI, an advanced extension of the [Alan-Leo-Wong/PCB-BVH](https://github.com/Alan-Leo-Wong/PCB-BVH/tree/master) project that integrates a user-friendly interface for real-time visualization and analysis of **2D PCB data** consisting of line segments and circle arcs. This tool not only enhances PCB design but also incorporates modern visualization technologies using OpenGL 4.0, Eigen for mathematical operations, and ImGui for an interactive graphical interface.

## Introduction

PCB-BVH-UI leverages the computational efficiency of Eigen and the rendering capabilities of OpenGL 4.0 to offer real-time collision detection and closest point query functionalities within a PCB layout. The intuitive UI developed with ImGui makes it accessible for users to interactively manipulate and analyze PCB data.

## Features

- **Real-time Collision Detection Animation**: Provides live visual feedback on collisions within the PCB layout, powered by OpenGL.
- **Interactive Closest Point Queries**: Facilitates dynamic closest point searches to aid in effective component placement and routing optimization.
- **User-Friendly Graphical Interface**: Built using ImGui, the interface simplifies complex data manipulations and enhances user engagement.
- **2D PCB Visualization**: Visualizes PCB data effectively with support for both line segments and arcs, ensuring a comprehensive geometric representation.

## Prerequisites

Ensure you have the following installed on your system before setting up PCB-BVH-UI:
- CMake 3.21 or higher
- A C++ compiler supporting C++20
- OpenGL 4.0

## Getting Started

To set up and run PCB-BVH-UI, follow these steps:

```bash
git clone https://github.com/Alan-Leo-Wong/PCB-BVH-UI.git
cd PCB-BVH-UI
mkdir build && cd build
cmake ..
cmake --build . -j your-core-num
```

## Usage

After building the executables, run them directly from the command line with the appropriate data file as an argument:

- For **collision detection:** `./test_cd <path_to_pcb_data_file>`
- For **closest point queries:** `./test_cp <path_to_pcb_data_file>`

We provide two test data in the `test/pcb_data` directory:

- `initial_normal.txt`: A standard complexity PCB design.
- `initial_hard.txt`: A high complexity PCB design for more rigorous testing.

Interact with the UI to visualize results in real-time.