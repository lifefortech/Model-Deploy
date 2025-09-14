#ifndef TRT_SEGMENTATION_H
#define TRT_SEGMENTATION_H

#ifdef _WIN32
    #ifdef TRT_SEG_API_EXPORTS
        #define TRT_SEG_API __declspec(dllexport)
    #else
        #define TRT_SEG_API __declspec(dllimport)
    #endif
#else
    #define TRT_SEG_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 使用一个不透明指针来隐藏内部 C++ 实现
typedef void* TRT_SEG_HANDLE;

/**
 * @brief 创建一个分割器实例
 * @return 返回一个句柄，后续操作都需要这个句柄。如果失败则返回 nullptr。
 */
TRT_SEG_API TRT_SEG_HANDLE create_segmentation_instance();

/**
 * @brief 销毁分割器实例并释放所有资源
 * @param handle create_segmentation_instance 返回的句柄
 */
TRT_SEG_API void destroy_segmentation_instance(TRT_SEG_HANDLE handle);

/**
 * @brief 初始化 TensorRT 引擎
 * @param handle 实例句柄
 * @param engine_path .engine 模型的绝对路径
 * @return 0 表示成功, 其他值表示失败
 */
TRT_SEG_API int init_engine(TRT_SEG_HANDLE handle, const char* engine_path);

/**
 * @brief 对输入的图像执行语义分割
 * @param handle 实例句柄
 * @param image_path 输入图像的绝对路径
 * @param output_mask_path 输出分割掩码图像的保存路径
 * @return 0 表示成功, 其他值表示失败
 */
TRT_SEG_API int run_inference(TRT_SEG_HANDLE handle, const char* image_path, const char* output_mask_path);


#ifdef __cplusplus
}
#endif

#endif // TRT_SEGMENTATION_H
