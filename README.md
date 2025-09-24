# TensorRT Segmentation DLL

This project wraps a TensorRT semantic segmentation engine into a DLL with a C-style API for easy use in other applications.

## Prerequisites

- NVIDIA GPU with appropriate driver
- CUDA Toolkit
- cuDNN
- TensorRT
- OpenCV
- CMake
- A C++ compiler (e.g., Visual Studio on Windows)

## How to Build

1.  **Configure Paths**: Open `CMakeLists.txt` and set the correct paths for `TENSORRT_DIR`, `CUDA_TOOLKIT_ROOT_DIR`, and `OpenCV_DIR`.

2.  **Create a build directory**:
    ```bash
    mkdir build
    cd build
    ```

3.  **Run CMake and Build**:

    **On Windows (with Visual Studio 2019):**
    ```bash
    # Generate solution files
    cmake .. -G "Visual Studio 17 2022" -A x64

    # Build the project (Debug or Release)
    cmake --build . --config Release
    ```

    **On Linux:**
    ```bash
    # Generate Makefiles
    cmake ..

    # Build the project
    make
    ```

4.  **Find Artifacts**: The compiled DLL (`trt_segmentation.dll`), LIB (`trt_segmentation.lib`), and header (`trt_segmentation.h`) will be in the `build/lib`, `build/bin`, and `include` directories respectively.

## TODO

The user of this template needs to implement the specific image pre-processing and mask post-processing logic in `src/trt_segmentation.cpp` according to their model's requirements.