
#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <numeric>
#include <functional>

#include "NvInfer.h"
#include "NvInferRuntime.h"
#include <cuda_runtime.h>

#include <opencv2/opencv.hpp>


class Logger : public nvinfer1::ILogger {
    void log(Severity severity, const char* msg) noexcept override {
        // if (severity <= Severity::kWARNING) {
        //     std::cout << msg << std::endl;
        // }
    }
};

class TRTSegmentation {
public:
    TRTSegmentation() = default;
    ~TRTSegmentation();

    int init(const std::string& engine_path);
    int run(const std::string& image_path, const std::string& output_mask_path);

private:
    void preprocess(const cv::Mat& image);
    void postprocess(cv::Mat& mask, const nvinfer1::Dims& dims);

    Logger logger_;
    std::unique_ptr<nvinfer1::IRuntime> runtime_;
    std::unique_ptr<nvinfer1::ICudaEngine> engine_;
    std::unique_ptr<nvinfer1::IExecutionContext> context_;

    std::vector<void*> buffers_;
    nvinfer1::Dims input_dims_;
    nvinfer1::Dims output_dims_;
    int input_binding_index_ = -1;
    int output_binding_index_ = -1;
    std::string input_tensor_name_;
    std::string output_tensor_name_;

    std::vector<float> host_input_;
    std::vector<float> host_output_;
};
