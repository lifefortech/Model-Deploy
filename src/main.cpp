#include <iostream>
#include "trt_segmentation.h"

int main() {
    std::cout << "--- Testing TRT Segmentation DLL ---" << std::endl;

    TRT_SEG_HANDLE handle = create_segmentation_instance();
    if (!handle) {
        std::cerr << "Failed to create segmentation instance." << std::endl;
        return -1;
    }
    std::cout << "Instance created." << std::endl;

    // 确保 .engine 文件位于可执行文件所在的目录，或提供绝对路径
    const char* engine_path = "deeplabv3_dynamic.engine";
    std::cout << "Attempting to initialize engine: " << engine_path << std::endl;
    
    int init_result = init_engine(handle, engine_path);
    if (init_result != 0) {
        std::cerr << "Failed to initialize engine. Make sure the engine file exists and is valid." << std::endl;
        destroy_segmentation_instance(handle);
        return -1;
    }
    std::cout << "Engine initialized successfully." << std::endl;

    // 确保输入图片文件位于可执行文件所在的目录，或提供绝对路径
    const char* input_image_path = "input.jpg";
    const char* output_mask_path = "output.png"; // 输出将保存到这里

    std::cout << "Attempting to run inference on: " << input_image_path << std::endl;
    
    int inference_result = run_inference(handle, input_image_path, output_mask_path);

    if (inference_result != 0) {
        std::cerr << "Inference failed. Check input image path and model compatibility." << std::endl;
    } else {
        std::cout << "Inference completed successfully. Output saved to " << output_mask_path << std::endl;
    }

    destroy_segmentation_instance(handle);
    std::cout << "Instance destroyed." << std::endl;

    std::cout << "--- Test Finished ---" << std::endl;
    return inference_result;
}
