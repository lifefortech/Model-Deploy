#include "trt_segmentation.h"
#include "../include/trt_segmentation_impl.h"

extern "C" {

TRT_SEG_API TRT_SEG_HANDLE create_segmentation_instance() {
    // new 返回一个 TRTSegmentation* 指针，我们将其转换为不透明的 void*
    return reinterpret_cast<TRT_SEG_HANDLE>(new TRTSegmentation());
}

TRT_SEG_API void destroy_segmentation_instance(TRT_SEG_HANDLE handle) {
    if (handle) {
        // 将 void* 转回 TRTSegmentation* 并删除
        delete reinterpret_cast<TRTSegmentation*>(handle);
    }
}

TRT_SEG_API int init_engine(TRT_SEG_HANDLE handle, const char* engine_path) {
    if (!handle || !engine_path) return -1;
    TRTSegmentation* instance = reinterpret_cast<TRTSegmentation*>(handle);
    return instance->init(engine_path);
}

TRT_SEG_API int run_inference(TRT_SEG_HANDLE handle, const char* image_path, const char* output_mask_path) {
    if (!handle || !image_path || !output_mask_path) return -1;
    TRTSegmentation* instance = reinterpret_cast<TRTSegmentation*>(handle);
    return instance->run(image_path, output_mask_path);
}

}
