cmake_minimum_required(VERSION 3.18)
project(TRTSegmentationDLL CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动查找依赖
# 确保 CUDA_PATH, OpenCV_DIR, TENSORRT_ROOT 已经设置在环境变量中
find_package(CUDA REQUIRED)
find_package(OpenCV REQUIRED)


# 查找 TensorRT
set(TENSORRT_DIR "D:/Environment/TensorRT-10.7.0.23")
message(STATUS "Using hardcoded TensorRT path: ${TENSORRT_DIR}")


include_directories(${TENSORRT_DIR}/include)
link_directories(${TENSORRT_DIR}/lib)

# 定义 TensorRT 的库
set(TENSORRT_LIBRARIES nvinfer_10 nvonnxparser_10)


# 定义 DLL
add_library(trt_segmentation SHARED
    src/trt_segmentation.cpp
    src/dll_interface.cpp
)

# 添加包含目录
target_include_directories(trt_segmentation PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${CUDA_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(trt_segmentation PRIVATE
    ${CUDA_CUDART_LIBRARY}
    ${TENSORRT_LIBRARIES}
    ${OpenCV_LIBS}
)

# 定义 Windows DLL 导出宏
target_compile_definitions(trt_segmentation PRIVATE TRT_SEG_API_EXPORTS)

# --- 安装和输出配置 ---
# 将所有构建产物（exe, dll, lib）都输出到同一个目录中，
# 以避免在运行时出现找不到 DLL 的问题。
# 目录结构将是 build/Debug 或 build/Release。
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

# 安装规则 (可选)
install(TARGETS trt_segmentation
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES include/trt_segmentation.h DESTINATION include)

message(STATUS "Project: ${PROJECT_NAME}")

message(STATUS "CUDA libraries: ${CUDA_LIBRARIES}")
message(STATUS "TensorRT include path: ${TENSORRT_DIR}/include")

# --- 可选的测试执行文件 ---
add_executable(trt_test src/main.cpp)

# 链接到我们的 DLL
target_link_libraries(trt_test PRIVATE trt_segmentation)

# 为测试程序也添加OpenCV的包含目录，以防需要
target_include_directories(trt_test PRIVATE ${OpenCV_INCLUDE_DIRS})

message(STATUS "Added test executable: trt_test")
