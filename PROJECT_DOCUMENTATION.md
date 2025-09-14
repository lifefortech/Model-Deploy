# 项目文档：TensorRT 语义分割 DLL

## 1. 项目概述

本项目是一个使用 C++ 和 TensorRT 实现的语义分割库。它被编译成一个动态链接库 (DLL)，可以被其他应用程序调用，用于对图像进行推理并生成分割掩码。

项目主要包含三个部分：
1.  **核心推理库 (`trt_segmentation`)**: 一个实现了模型加载、图像预处理、TensorRT 推理和后处理的 C++ 类。
2.  **C 语言风格的 DLL 接口**: 为了方便不同语言调用，核心库被封装在一层 C 风格的 API 后面。
3.  **测试程序 (`trt_test`)**: 一个简单的可执行文件，用于加载 DLL 并测试其功能。

---

## 2. 文件结构和代码说明

### `CMakeLists.txt`
- **作用**: 项目的构建配置文件，使用 CMake 来管理编译过程。
- **关键点**:
    - `project(TRTSegmentationDLL CXX C)`: 定义项目名称和语言。
    - `find_package(CUDA REQUIRED)` 和 `find_package(OpenCV REQUIRED)`: 自动查找系统中已安装的 CUDA 和 OpenCV 依赖。
    - `set(TENSORRT_DIR ...)`: **硬编码**了 TensorRT 的路径。如果环境变更，需要修改此路径。
    - `add_library(trt_segmentation SHARED ...)`: 定义了核心的动态链接库 `trt_segmentation`，包含了 `trt_segmentation.cpp` 和 `dll_interface.cpp`。
    - `target_link_libraries(...)`: 将库链接到 CUDA, TensorRT 和 OpenCV。
    - `add_executable(trt_test src/main.cpp)`: 定义了用于测试的可执行文件 `trt_test`。
    - `target_link_libraries(trt_test PRIVATE trt_segmentation)`: 将测试程序链接到我们自己生成的 DLL。

### `include/trt_segmentation.h`
- **作用**: 这是提供给外部使用的公共头文件，定义了 DLL 的导出函数。
- **关键点**:
    - `TRT_SEG_API`: 使用 `__declspec(dllexport)` 和 `__declspec(dllimport)` 宏来控制函数的导入和导出，这是 Windows DLL 的标准做法。
    - `extern "C"`: 确保函数以 C 语言的方式导出，避免 C++ 的名字修饰 (name mangling)，从而让其他语言（如 C#, Python）可以方便地调用。
    - `TRT_SEG_HANDLE`: 使用一个不透明的 `void*` 指针作为句柄，向用户隐藏了内部 C++ 类的实现细节，这是一种良好的 API 设计实践。
    - 定义了四个核心 API 函数：`create_...`, `destroy_...`, `init_engine`, `run_inference`。

### `include/trt_segmentation_impl.h`
- **作用**: 这是项目内部使用的私有头文件，定义了核心 C++ 类 `TRTSegmentation` 的结构。
- **关键点**:
    - `class TRTSegmentation`: 封装了所有与 TensorRT 相关的对象（Runtime, Engine, Context）和操作逻辑。
    - `std::unique_ptr`: 使用智能指针来自动管理 TensorRT 对象的生命周期，避免内存泄漏。
    - `host_input_`, `host_output_`: 定义了用于存放预处理输入数据和模型输出数据的 CPU 侧向量。
    - `buffers_`: 存放指向 GPU 内存的指针的向量。

### `src/dll_interface.cpp`
- **作用**: 这是 C++ 核心逻辑和 C 风格 API 之间的桥梁。
- **关键点**:
    - `reinterpret_cast<TRT_SEG_HANDLE>(new TRTSegmentation())`: 在 `create_...` 函数中，创建一个 C++ 对象并将其指针“隐藏”在一个 `void*` 句柄中返回给用户。
    - `delete reinterpret_cast<TRTSegmentation*>(handle)`: 在 `destroy_...` 函数中，将 `void*` 句柄转回原始的 C++ 对象指针并安全地删除它。
    - 其他函数负责将句柄转换回对象指针，并调用对应的 C++ 成员函数。

### `src/trt_segmentation.cpp`
- **作用**: 这是项目的核心，包含了所有功能的具体实现。
- **关键点**:
    - `init()`: 从文件加载 `.engine`，反序列化创建 TensorRT 引擎和执行上下文。
    - `preprocess()`: 实现图像的预处理。**这是我们修复的关键点之一**。
    - `postprocess()`: 实现模型输出的后处理，将模型的原始输出（通常是每个像素的类别得分）转换成一张可视化的黑白掩码图。
    - `run()`: 串联起所有操作的中心函数。它负责设置动态尺寸、分配/释放GPU内存、调用预处理、执行推理、调用后处理以及保存最终图像。

### `src/main.cpp`
- **作用**: 一个简单的客户端程序，用于演示如何调用 DLL 提供的 API。
- **关键点**:
    - 调用流程清晰地展示了 API 的标准用法：`create` -> `init` -> `run` -> `destroy`。

---

## 3. “黑图”问题诊断与解决

我们在使用中遇到了一个典型的问题：**程序可以正常运行结束，不报告任何错误，但生成的输出图片 `output.png` 是一张纯黑色的图像。**

### 3.1 问题诊断过程

1.  **初步怀疑**: 首先怀疑是后处理 `postprocess` 函数逻辑有问题，例如像素赋值错误。通过检查代码，发现逻辑是 `(max_idx > 0) ? 255 : 0`，这意味着只有当模型预测的类别索引大于0时，像素才会被设为白色。黑图意味着 `max_idx` 总是0。

2.  **检查模型输出**: 为了验证 `max_idx` 是否总是0，我们在 `postprocess` 函数中加入了调试代码，打印出模型推理后 `host_output_` 缓冲区中的原始数据。**调试输出显示，该缓冲区中的数据确实全部为0。**

3.  **检查模型输入**: 模型输出为0，可能是因为输入就是0。我们接着在 `preprocess` 函数后加入了调试代码，打印预处理后的输入数据 `host_input_`。**调试输出显示，输入数据是正常的、非零的浮点数。**

4.  **定位推理环节**: 既然输入正常而输出为0，问题几乎可以肯定出在 `TensorRT` 的推理执行步骤。我们尝试了两种不同的推理调用 API (`enqueueV3` 和 `executeV2`)，但问题依旧。

5.  **最终突破**: 我们在代码中加入了对 CUDA API 调用的显式错误检查。再次运行时，程序终于报错：`Error: CUDA H2D memcpy failed: invalid argument`。这个错误发生在从 CPU 向 GPU 复制输入数据时。

### 3.2 问题根源

我们最终定位到两个独立的根源性问题：

1.  **GPU 内存分配错误 (导致程序崩溃的直接原因)**: 
    - **现象**: `cudaMemcpy` 调用返回 `invalid argument` 错误。
    - **原因**: 我们的模型支持动态输入尺寸。代码最初在 `init()` 函数中就分配了 GPU 内存。但在那个时间点，由于不知道后续会使用多大的图片，TensorRT 返回的维度信息是 `[1, 3, -1, -1]`，导致程序只分配了一个极小的 GPU 缓冲区。随后在 `run()` 函数中，当程序试图将一个 `512x512` 的完整图片数据复制到这个小缓冲区时，CUDA 检测到目标缓冲区大小远小于要复制的数据量，因此返回了 `invalid argument` 错误。

2.  **图像归一化方法错误 (导致黑图的根本原因)**:
    - **现象**: 即使解决了崩溃问题，输出的图像也可能是黑色的。
    - **原因**: 深度学习模型在训练时，其输入数据都经过了特定的归一化处理。我们的模型（DeepLabV3）是在 ImageNet 数据集上预训练的，要求输入图像在 `[0,1]` 范围内，并根据特定的均值 `[0.485, 0.456, 0.406]` 和标准差 `[0.229, 0.224, 0.225]` 进行标准化。而代码最初只做了简单的 `/ 255.0` 处理，没有减去均值和除以标准差。这种数据分布的巨大差异导致模型无法正确识别图像特征，从而输出了无效的全零结果。

### 3.3 解决方案

针对以上两个问题，我们对 `src/trt_segmentation.cpp` 进行了最终修改：

1.  **修正内存分配**: 
    - 将 `init()` 函数中的所有 `cudaMalloc` 调用移除。
    - 将 GPU 内存的分配逻辑整体移动到 `run()` 函数中。在调用 `context_->setInputShape()` 确定了具体的输入尺寸之后，再使用 `context_->getTensorShape()` 获取精确的输入和输出维度，并以此分配正确大小的 `buffers_`。
    - 在每次 `run` 的开始，先释放上一次运行时分配的 `buffers_`，以支持连续调用。

2.  **修正归一化**: 
    - 修改 `preprocess()` 函数，在将像素值缩放到 `[0,1]` 范围后，继续减去对应的均值，再除以标准差，完成正确的标准化流程。

通过这两处关键修改，项目最终得以成功运行，并生成了正确的分割掩码图像。