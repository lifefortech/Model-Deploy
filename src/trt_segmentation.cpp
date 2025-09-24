#include "../include/trt_segmentation_impl.h"

TRTSegmentation::~TRTSegmentation() {
    for (void* buffer : buffers_) {
        cudaFree(buffer);
    }
}

int TRTSegmentation::init(const std::string& engine_path) {
    std::ifstream engine_file(engine_path, std::ios::binary);
    if (!engine_file.is_open()) {
        std::cerr << "Error: Could not open engine file: " << engine_path << std::endl;
        return -1;
    }

    engine_file.seekg(0, engine_file.end);
    long int fsize = engine_file.tellg();
    engine_file.seekg(0, engine_file.beg);

    std::vector<char> engine_data(fsize);
    engine_file.read(engine_data.data(), fsize);

    this->runtime_.reset(nvinfer1::createInferRuntime(this->logger_));
    if (!this->runtime_) {
        std::cerr << "Error: Failed to create TensorRT runtime." << std::endl;
        return -1;
    }

    this->engine_.reset(this->runtime_->deserializeCudaEngine(engine_data.data(), fsize));
    if (!this->engine_) {
        std::cerr << "Error: Failed to deserialize CUDA engine." << std::endl;
        return -1;
    }

    this->context_.reset(this->engine_->createExecutionContext());
    if (!this->context_) {
        std::cerr << "Error: Failed to create execution context." << std::endl;
        return -1;
    }

    // Get tensor names and indices, but defer buffer allocation to run()
    for (int i = 0; i < this->engine_->getNbIOTensors(); ++i) {
        const char* tensor_name = this->engine_->getIOTensorName(i);
        if (this->engine_->getTensorIOMode(tensor_name) == nvinfer1::TensorIOMode::kINPUT) {
            this->input_tensor_name_ = tensor_name;
            this->input_binding_index_ = i;
        } else {
            this->output_tensor_name_ = tensor_name;
            this->output_binding_index_ = i;
        }
    }

    if (this->input_tensor_name_.empty() || this->output_tensor_name_.empty()) {
        std::cerr << "Error: Could not find input or output tensors." << std::endl;
        return -1;
    }

    return 0;
}

void TRTSegmentation::preprocess(const cv::Mat& image) {
    cv::Mat float_image;
    image.convertTo(float_image, CV_32FC3, 1.0 / 255.0);

    // Normalization with mean and std
    const float mean[3] = {0.485f, 0.456f, 0.406f};
    const float std[3] = {0.229f, 0.224f, 0.225f};

    int height = image.rows;
    int width = image.cols;
    this->host_input_.resize(1 * 3 * height * width);
    float* host_data_ptr = this->host_input_.data();

    for (int c = 0; c < 3; ++c) {
        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                // Assuming BGR input from cv::imread, normalize and convert to CHW
                float val = float_image.at<cv::Vec3f>(h, w)[c];
                host_data_ptr[c * (height * width) + h * width + w] = (val - mean[c]) / std[c];
            }
        }
    }
}

void TRTSegmentation::postprocess(cv::Mat& mask, const nvinfer1::Dims& dims) {
    int num_classes = dims.d[1];
    int height = dims.d[2];
    int width = dims.d[3];
    
    mask = cv::Mat(height, width, CV_8UC1);
    
    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            float max_val = -std::numeric_limits<float>::infinity();
            int max_idx = 0;
            for (int c = 0; c < num_classes; ++c) {
                // Note: The model output might be int64 or float32 depending on the model.
                // Casting to float here for general case.
                float val = static_cast<float>(this->host_output_[c * (height * width) + h * width + w]);
                if (val > max_val) {
                    max_val = val;
                    max_idx = c;
                }
            }
            mask.at<uchar>(h, w) = (max_idx > 0) ? 255 : 0;
        }
    }
}

int TRTSegmentation::run(const std::string& image_path, const std::string& output_mask_path) {
    cv::Mat image = cv::imread(image_path, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "Error: Could not read input image: " << image_path << std::endl;
        return -1;
    }

    const int original_height = image.rows;
    const int original_width = image.cols;

    // --- 修改开始 ---

    // 1. 定义明确的目标高度和宽度
    const int target_height = 256;
    const int target_width = 2048;

    cv::Mat resized_image;
    // 2. 调用 cv::resize 时，cv::Size 的参数顺序为 (宽度, 高度)
    cv::resize(image, resized_image, cv::Size(target_width, target_height));

    // 3. 设置 TensorRT 的输入维度，顺序为 (N, C, H, W)，即 (批量, 通道, 高度, 宽度)
    if (!this->context_->setInputShape(this->input_tensor_name_.c_str(), nvinfer1::Dims4{1, 3, target_height, target_width})) {
        std::cerr << "Error: Failed to set input shape." << std::endl;
        return -1;
    }

    // Allocate/Reallocate buffers now that we have the exact dimensions
    for (void* buffer : buffers_) { // Free any previous allocations
        cudaFree(buffer);
    }
    buffers_.assign(this->engine_->getNbIOTensors(), nullptr);

    for (int i = 0; i < this->engine_->getNbIOTensors(); ++i) {
        const char* tensor_name = this->engine_->getIOTensorName(i);
        nvinfer1::Dims dims = this->context_->getTensorShape(tensor_name);
        size_t vol = 1;
        for (int j = 0; j < dims.nbDims; ++j) vol *= dims.d[j];
        cudaMalloc(&this->buffers_[i], vol * sizeof(float)); // Assuming float for both
    }

    this->preprocess(resized_image);

    cudaMemcpy(this->buffers_[this->input_binding_index_], this->host_input_.data(), this->host_input_.size() * sizeof(float), cudaMemcpyHostToDevice);

    if (!this->context_->executeV2(this->buffers_.data())) {
        std::cerr << "Error: Failed to execute inference." << std::endl;
        return -1;
    }

    auto output_dims = this->context_->getTensorShape(this->output_tensor_name_.c_str());
    size_t output_size = 1;
    for(int j=0; j < output_dims.nbDims; ++j) output_size *= output_dims.d[j];
    
    // The model output might be int64, let's assume float for now as per buffer allocation
    // but be mindful of the actual model output type.
    host_output_.resize(output_size);
    cudaMemcpy(this->host_output_.data(), this->buffers_[this->output_binding_index_], output_size * sizeof(float), cudaMemcpyDeviceToHost);

    cv::Mat output_mask;
    this->postprocess(output_mask, output_dims);

    cv::Mat final_mask;
    cv::resize(output_mask, final_mask, cv::Size(original_width, original_height), 0, 0, cv::INTER_NEAREST);

    if (!cv::imwrite(output_mask_path, final_mask)) {
        std::cerr << "Error: Could not save output mask." << std::endl;
        return -1;
    }

    return 0;
}
