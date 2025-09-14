/* stb_image - v2.27 - public domain image loader - http://nothings.org/stb
                                  no warranty implied; use at your own risk

   Do this:
      #define STB_IMAGE_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.

   // i.e. it should look like this:
   #include ...
   #include ...
   #define STB_IMAGE_IMPLEMENTATION
   #include "stb_image.h"

   You can #define STBI_ASSERT(x) before the #include to avoid using assert.h.
   And #define STBI_MALLOC, STBI_REALLOC, and STBI_FREE to avoid using malloc,realloc,free


   QUICK NOTES:
      Primarily of interest to game developers and other people who can
          avoid problematic images and formats JPEG-progressive (see below)

      JPEG baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib)
      PNG 1/2/4/8/16-bit-per-channel

      TGA (not sure what subset, if a subset)
      BMP non-1bpp, non-RLE
      PSD (composited view only, no extra channels, 8/16 bit-per-channel)

      GIF (*comp always reports as 4-channel)
      HDR (radiance rgbE format)
      PIC (Softimage PIC)
      PNM (PPM and PGM binary only)

      Animated GIF still needs a proper API, but here's one way to do it:
          http://gist.github.com/urraka/685d9a6340b26b830d49

      - decode from memory or through FILE (define STBI_NO_STDIO to remove code)
      - decode from arbitrary I/O callbacks
      - SIMD acceleration on x86/x64 (SSE2) and ARM (NEON)

   Full documentation under "DOCUMENTATION" below.


LICENSE

  See end of file for license information.

RECENT REVISION HISTORY:

      2.27  (2021-07-11) document stbi_info better, 16-bit PNM support, bug fixes
      2.26  (2020-07-13) many minor fixes
      2.25  (2020-04-03) fix security vulnerability
      2.24  (2020-02-03) fix security vulnerability
      2.23  (2019-08-11) fix clang static analysis warning
      2.22  (2019-03-04) gif fixes, fix warnings
      2.21  (2019-02-25) fix typo in comment
      2.20  (2019-02-07) support utf8 filenames in Windows; fix warnings; fix disclaimers
      2.19  (2018-02-11) fix warning
      2.18  (2018-01-30) fix warnings
      2.17  (2018-01-29) bugfix, 1-bit BMP, 16-bitness query, fix warnings
      2.16  (2017-07-23) all functions have 16-bit variants; optimizations; bugfixes
      2.15  (2017-03-18) fix png-1,2,4; all Imagenet JPGs; no runtime SSE detection on GCC
      2.14  (2017-03-03) remove deprecated STBI_JPEG_OLD; fix several bugs; support 16-bit PSD
      2.13  (2016-12-04) experimental 16-bit API, only for PNG so far; fix bug
      2.12  (2016-04-02) fix typo in 2.11 PSD fix
      2.11  (2016-04-02) 16-bit PNG fix; enable SSE2 SIMD by default; fix PSD bug
      2.10  (2016-01-22) avoid warning introduced in 2.09
      2.09  (2016-01-16) 16-bit TGA; comments in PNM files; STBI_REALLOC_SIZED

   See bottom of file for full revision history.


 ============================    Contributors    =========================

 Image formats                          Extensions, features
    Sean Barrett (jpeg, png, bmp)          Jetro Lauha (stbi_info)
    Nicolas Schulz (hdr, tga)              Martin Sladecek (gif read/write)
    Jonathan Dummer (psd)                  Shawn Vokus (stbi_jpeg_write)
    Jean-Marc Valin (pic)                  Guy Ellis (stbi_jpeg_write)
    Christpher M. Kohlhoff (pnm)           (jpeg write)
    Dave Moore (gif)                       (png write)
    Aarni Koskela (gif)                    (bmp write)
    Thatcher Ulrich (psd)                  (tga write)
    Ken Miller (jpeg)                      (hdr write)
    Blazej Dariusz Roszkowski (jpeg)       (pic write)
    Artur Tomala (jpeg)                    (pnm write)
    Arseny Kapoulkine (jpeg)
    John-Mark Allen (jpeg)
    Chad Austin (jpeg)

 SIMD support
    Fabian "ryg" Giesen
    Arseny Kapoulkine

 Bug fixes & warning fixes
    Marc LeBlanc            David Woo          Guillaume George   Martins Mozeiko
    Christpher M. Kohlhoff  Bjoern Nilson      Ken Miller         Julian Wiesener
    Jonathan Dummer         RYG                Sean Barrett       Johan Duparc
    Manish Goregaokar       Hexaddecimal       Jean-Marc Valin    Owen O'Malley
    Ronny Chevalier         Fabian Giesen      Adam Hooper        Sebastian Schmidt
    John-Mark Allen         Velko Ivanov       Gero Mueller       Ignacio Castano
    Ronny Chevalier                          (many others)

*/

#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

// DOCUMENTATION
//
// Limitations:
//    - no 12-bit-per-channel JPEG
//    - no JPEGs with arithmetic coding
//    - GIF always returns *comp=4
//
// Basic usage (see HDR discussion below for HDR usage):
//    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... 'n' can be 1, 2, 3, or 4. Objects with other 'n' are forced to 4...
//    // ... or you can force the components per pixel yourself:
//    data = stbi_load(filename, &x, &y, &n, 1); // force 1 component (grayscale)
//    data = stbi_load(filename, &x, &y, &n, 2); // force 2 components (grayscale+alpha)
//    data = stbi_load(filename, &x, &y, &n, 3); // force 3 components (rgb)
//    data = stbi_load(filename, &x, &y, &n, 4); // force 4 components (rgba)
//    // ...
//    stbi_image_free(data)
//
// You can query the size of an image without loading it by passing NULL for
// the data argument in any of the stbi_load_* functions:
//
//    int x,y,n;
//    stbi_info(filename, &x, &y, &n);
//
// Or, if you're using the FILE interface:
//
//   FILE *f = fopen(filename, "rb");
//   stbi_info_from_file(f, &x, &y, &n);
//
// Or, you can use the callback interface:
//
//   stbi_info_from_callbacks(&io_callback_struct, &x, &y, &n);
//
// You can get floating point data instead of unsigned char data by using
// stbi_loadf. You get back float * data from it.
//
// You can get 16-bit integer data instead of unsigned char data by using
// stbi_load_16. You get back stbi_us * data from it.
//
// If you load LDR images through any of the HDR functions, they will be
// promoted to HDR format, i.e. float-per-component.
//
// Supported formats:
//    JPEG baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib)
//    PNG 1/2/4/8/16-bit-per-channel
//    TGA (not sure what subset, if a subset)
//    BMP non-1bpp, non-RLE
//    PSD (composited view only, no extra channels, 8/16 bit-per-channel)
//    GIF
//    HDR (radiance rgbE format)
//    PIC (Softimage PIC)
//    PNM (PPM and PGM binary only)
//
// Animated GIF support:
//    The current GIF parser only supports loading single frames.
//
//    To read animated GIFs, use stbi_load_gif_from_memory, which returns the number
//    of frames. To get other information, use stbi_gif_info_from_memory. It is
//    still not a complete API, but it works in practice.
//
// Unwanted formats:
//    You can #define STBI_NO_JPEG to remove JPEG support.
//    You can #define STBI_NO_PNG to remove PNG support.
//    You can #define STBI_NO_BMP to remove BMP support.
//    You can #define STBI_NO_PSD to remove PSD support.
//    You can #define STBI_NO_TGA to remove TGA support.
//    You can #define STBI_NO_GIF to remove GIF support.
//    You can #define STBI_NO_HDR to remove HDR support.
//    You can #define STBI_NO_PIC to remove PIC support.
//    You can #define STBI_NO_PNM to remove PNM support.  (STBI_NO_PPM is a synonym)
//
// simd support:
//    stb_image uses simd instructions on x86/x64 and ARM NEON to accelerate
//    jpeg decoding. You can disable this behavior by defining STBI_NO_SIMD.
//
// Thread-safety:
//    stb_image is thread-safe, but stbi_failure_reason() is not.
//
// ZLIB client usage:
//    If you compile with STBI_NO_ZLIB, you must additionally provide a
//    stbi_zlib_decode_malloc_guesssize_headerflag() function, see the notes
//    in stbi_zlib_decode_malloc_guesssize_headerflag() for details.
//
// Memory management:
//    You can define STBI_MALLOC, STBI_REALLOC, and STBI_FREE to replace the
//    standard memory management functions. The memory returned by stbi_load
//    is owned by the caller and must be freed with stbi_image_free (which
//    calls STBI_FREE).
//
// UTF-8 filenames:
//    stb_image now tries to handle UTF-8 filenames on Windows. To do this,
//    it calls MultiByteToWideChar to convert the filename to a wchar_t string,
//    then calls _wfopen to open the file. If you want to disable this,
//    #define STBI_NO_WINDOWS_UTF8.
//
// HDR image support:
//    stb_image now supports loading HDR images in general, and Radiance's
//    RGBE format specifically. You can still load LDR images through the
//    HDR interface; they will be promoted to float and moved into a linear
//    color space.
//
//    To load an HDR image, use stbi_loadf() or stbi_loadf_from_memory().
//
//    The values returned by the HDR interface are floats in the range [0,1],
//    and are in linear color space.
//
//    To load an LDR image and have it automatically converted to HDR, you
//    can still use stbi_load(), but you must then convert the data to float
//    and then to linear color space.
//
//
// Special options:
//
//    STBI_NO_STDIO:
//        If you define this, stb_image will not use stdio file I/O at all.
//        You will need to provide your own I/O functions, see the documentation
//        for stbi_io_callbacks.
//
//    STBI_NO_LINEAR:
//        stb_image converts HDR images to linear color space by default.
//        If you want to disable this, #define STBI_NO_LINEAR.
//
//    STBI_NO_HDR:
//        You can disable HDR support entirely by defining this.
//
//    STBI_NO_GIF:
//        You can disable GIF support by defining this.
//
//    STBI_ONLY_JPEG:
//    STBI_ONLY_PNG:
//    STBI_ONLY_BMP:
//    STBI_ONLY_PSD:
//    STBI_ONLY_TGA:
//    STBI_ONLY_GIF:
//    STBI_ONLY_HDR:
//    STBI_ONLY_PIC:
//    STBI_ONLY_PNM:
//        You can define any of these to load only that format and not
//        the others. This may reduce code size.
//
//
// Stb-style headers:
//
//    Most stb-style headers have a documentation block like this one.
//    Each function is documented with a comment block above it.
//    Some headers provide a lot of macro configurability.
//
//    Some headers require you to define a macro to create the implementation.
//    For example, stb_image.h requires you to define STB_IMAGE_IMPLEMENTATION
//    in one C or C++ file before including it.
//
//
// I/O callbacks:
//
//    You can define STBI_NO_STDIO to disable the default file I/O functions.
//    If you do this, you will need to provide your own I/O functions.
//
//    struct stbi_io_callbacks
//    {
//       int (*read)(void *user, char *data, int size);
//       void (*skip)(void *user, int n);
//       int (*eof)(void *user);
//    };
//
//    You must then provide these functions to stb_image using
//    stbi_load_from_callbacks(), stbi_loadf_from_callbacks(), etc.
//
//
// Error handling:
//
//    stbi_failure_reason() returns a string with a descriptive error message
//    if the last operation failed. Otherwise, it returns NULL.
//
//
// 16-bit-per-channel images:
//
//    You can load 16-bit-per-channel images using stbi_load_16() and
//    stbi_load_16_from_memory(). These return stbi_us * data.
//
//
// Additional documentation:
//
//    You can find more documentation in the comments in the source code.
//
//
// Notes:
//
//    - stb_image is not thread-safe with respect to stbi_failure_reason().
//    - stb_image uses malloc/realloc/free for memory management.
//
//
// Revision history:
//
//    See end of file.
//

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif // STBI_NO_STDIO

#define STBI_VERSION 1

enum
{
    STBI_default = 0, // only used for desired_channels

    STBI_grey = 1,
    STBI_grey_alpha = 2,
    STBI_rgb = 3,
    STBI_rgb_alpha = 4
};

typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef STB_IMAGE_STATIC
#define STBIDEF static
#else
#define STBIDEF extern
#endif

    //////////////////////////////////////////////////////////////////////////////
    //
    // PRIMARY API - works on images of any type
    //

    //
    // load image by filename, open file, or memory buffer
    //

    typedef struct
    {
        int (*read)(void *user, char *data, int size); // fill 'data' with 'size' bytes.  return number of bytes actually read
        void (*skip)(void *user, int n);               // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
        int (*eof)(void *user);                        // returns nonzero if we are at end of file/data
    } stbi_io_callbacks;

    ////////////////////////////////////
    //
    // 8-bits-per-channel interface
    //

    STBIDEF stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
    STBIDEF stbi_uc *stbi_load_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *channels_in_file, int desired_channels);

#ifndef STBI_NO_STDIO
    STBIDEF stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
    STBIDEF stbi_uc *stbi_load_from_file(FILE *f, int *x, int *y, int *channels_in_file, int desired_channels);
// for stbi_load_from_file, file pointer is left pointing immediately after image
#endif

#ifndef STBI_NO_GIF
    STBIDEF stbi_uc *stbi_load_gif_from_memory(stbi_uc const *buffer, int len, int **delays, int *x, int *y, int *z, int *comp, int req_comp);
#endif

#ifdef STBI_WINDOWS_UTF8
    STBIDEF int stbi_convert_wchar_to_utf8(char *buffer, size_t bufferlen, const wchar_t *input);
#endif

    ////////////////////////////////////
    //
    // 16-bits-per-channel interface
    //

    STBIDEF stbi_us *stbi_load_16_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
    STBIDEF stbi_us *stbi_load_16_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *channels_in_file, int desired_channels);

#ifndef STBI_NO_STDIO
    STBIDEF stbi_us *stbi_load_16(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
    STBIDEF stbi_us *stbi_load_from_file_16(FILE *f, int *x, int *y, int *channels_in_file, int desired_channels);
#endif

    ////////////////////////////////////
    //
    // float-per-channel interface
    //

    STBIDEF float *stbi_loadf_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
    STBIDEF float *stbi_loadf_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *channels_in_file, int desired_channels);

#ifndef STBI_NO_STDIO
    STBIDEF float *stbi_loadf(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
    STBIDEF float *stbi_loadf_from_file(FILE *f, int *x, int *y, int *channels_in_file, int desired_channels);
#endif

#ifndef STBI_NO_HDR
    STBIDEF void stbi_hdr_to_ldr_gamma(float gamma);
    STBIDEF void stbi_hdr_to_ldr_scale(float scale);
#endif // STBI_NO_HDR

#ifndef STBI_NO_LINEAR
    STBIDEF void stbi_ldr_to_hdr_gamma(float gamma);
    STBIDEF void stbi_ldr_to_hdr_scale(float scale);
#endif // STBI_NO_LINEAR

    // stbi_is_hdr is tested before loading. If you don't want HDR support, you
    // can define STBI_NO_HDR and it will never be called.
    STBIDEF int stbi_is_hdr_from_callbacks(stbi_io_callbacks const *clbk, void *user);
    STBIDEF int stbi_is_hdr_from_memory(stbi_uc const *buffer, int len);
#ifndef STBI_NO_STDIO
    STBIDEF int stbi_is_hdr(char const *filename);
    STBIDEF int stbi_is_hdr_from_file(FILE *f);
#endif // STBI_NO_STDIO

    // get a failure reason if load fails
    STBIDEF const char *stbi_failure_reason(void);

    // free the loaded image -- this is just free()
    STBIDEF void stbi_image_free(void *retval_from_stbi_load);

    // get image dimensions & components without fully decoding
    STBIDEF int stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);
    STBIDEF int stbi_info_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp);
    STBIDEF int stbi_is_16_bit_from_memory(stbi_uc const *buffer, int len);
    STBIDEF int stbi_is_16_bit_from_callbacks(stbi_io_callbacks const *clbk, void *user);

#ifndef STBI_NO_STDIO
    STBIDEF int stbi_info(char const *filename, int *x, int *y, int *comp);
    STBIDEF int stbi_info_from_file(FILE *f, int *x, int *y, int *comp);
    STBIDEF int stbi_is_16_bit(char const *filename);
    STBIDEF int stbi_is_16_bit_from_file(FILE *f);
#endif

// for animated GIF caching, calls create an animation and point to it.
// passing returned pointer to free function frees the whole animation,
// passing it to step function steps through the animation
#ifndef STBI_NO_GIF
    typedef struct
    {
        int w, h;
        stbi_uc *out;
        int *delay;
    } stbi__gif;

    STBIDEF stbi__gif *stbi__gif_load(stbi_uc const *buffer, int len);
    STBIDEF void stbi__gif_free(stbi__gif *g);
    STBIDEF stbi_uc *stbi__gif_step(stbi__gif *g, int *d);
    STBIDEF int stbi__gif_info(stbi_uc const *buffer, int len, int *w, int *h);
#endif

    // ZLIB client - used by PNG, available for other purposes

    STBIDEF char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen);
    STBIDEF char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header);
    STBIDEF char *stbi_zlib_decode_malloc(const char *buffer, int len, int *outlen);
    STBIDEF int stbi_zlib_decode_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);
    // note: the above functions should be used for zlib-style decompression,
    // NOT deflate-style decompression.

    STBIDEF char *stbi_zlib_decode_noheader_malloc(const char *buffer, int len, int *outlen);
    STBIDEF int stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

#ifdef __cplusplus
}
#endif

//
//
////   end header file   /////////////////////////////////////////////////////
#endif // STBI_INCLUDE_STB_IMAGE_H

#ifdef STB_IMAGE_IMPLEMENTATION

#if defined(STBI_ONLY_JPEG) || defined(STBI_ONLY_PNG) || defined(STBI_ONLY_BMP) || defined(STBI_ONLY_TGA) || defined(STBI_ONLY_GIF) || defined(STBI_ONLY_PSD) || defined(STBI_ONLY_HDR) || defined(STBI_ONLY_PIC) || defined(STBI_ONLY_PNM) || defined(STBI_ONLY_ZLIB)
#ifndef STBI_ONLY_JPEG
#define STBI_NO_JPEG
#endif
#ifndef STBI_ONLY_PNG
#define STBI_NO_PNG
#endif
#ifndef STBI_ONLY_BMP
#define STBI_NO_BMP
#endif
#ifndef STBI_ONLY_PSD
#define STBI_NO_PSD
#endif
#ifndef STBI_ONLY_TGA
#define STBI_NO_TGA
#endif
#ifndef STBI_ONLY_GIF
#define STBI_NO_GIF
#endif
#ifndef STBI_ONLY_HDR
#define STBI_NO_HDR
#endif
#ifndef STBI_ONLY_PIC
#define STBI_NO_PIC
#endif
#ifndef STBI_ONLY_PNM
#define STBI_NO_PNM
#endif
#endif

#if defined(STBI_NO_PNG) && !defined(STBI_SUPPORT_ZLIB) && !defined(STBI_NO_ZLIB)
#define STBI_NO_ZLIB
#endif

#include <stdarg.h>
#include <stddef.h> // ptrdiff_t on osx
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if !defined(STBI_NO_LINEAR) || !defined(STBI_NO_HDR)
#include <math.h> // ldexp, pow
#endif

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif

#ifndef STBI_ASSERT
#include <assert.h>
#define STBI_ASSERT(x) assert(x)
#endif

#ifdef __cplusplus
#define STBI_EXTERN extern "C"
#else
#define STBI_EXTERN extern
#endif

#ifndef _MSC_VER
#ifdef __cplusplus
#define stbi_inline inline
#else
#define stbi_inline
#endif
#else
#define stbi_inline __forceinline
#endif

#ifndef STBI_NO_THREAD_LOCALS
#if defined(__cplusplus) && __cplusplus >= 201103L
#define STBI_THREAD_LOCAL thread_local
#elif defined(__GNUC__) && __GNUC__ >= 5
#define STBI_THREAD_LOCAL __thread
#elif defined(_MSC_VER)
#define STBI_THREAD_LOCAL __declspec(thread)
#endif
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
typedef unsigned short stbi__uint16;
typedef signed short stbi__int16;
typedef unsigned int stbi__uint32;
typedef signed int stbi__int32;
#else
#include <stdint.h>
typedef uint16_t stbi__uint16;
typedef int16_t stbi__int16;
typedef uint32_t stbi__uint32;
typedef int32_t stbi__int32;
#endif

// should produce compiler error if size is wrong
typedef unsigned char validate_uint32[sizeof(stbi__uint32) == 4 ? 1 : -1];

#ifdef _MSC_VER
#define STBI_NOTUSED(v) (void)(v)
#else
#define STBI_NOTUSED(v) (void)sizeof(v)
#endif

#ifdef _MSC_VER
#define STBI_HAS_LROTL
#endif

#ifdef STBI_HAS_LROTL
#define stbi_lrot(x, y) _lrotl(x, y)
#else
#define stbi_lrot(x, y) (((x) << (y)) | ((x) >> (32 - (y))))
#endif

#if defined(STBI_MALLOC) && defined(STBI_FREE) && (defined(STBI_REALLOC) || defined(STBI_REALLOC_SIZED))
// ok
#elif !defined(STBI_MALLOC) && !defined(STBI_FREE) && !defined(STBI_REALLOC) && !defined(STBI_REALLOC_SIZED)
// ok
#else
#error "Must define all or none of STBI_MALLOC, STBI_FREE, and STBI_REALLOC (or STBI_REALLOC_SIZED)."
#endif

#ifndef STBI_MALLOC
#define STBI_MALLOC(sz) malloc(sz)
#define STBI_REALLOC(p, newsz) realloc(p, newsz)
#define STBI_FREE(p) free(p)
#endif

#ifndef STBI_REALLOC_SIZED
#define STBI_REALLOC_SIZED(p, oldsz, newsz) STBI_REALLOC(p, newsz)
#endif

// x86/x64 detection
#if defined(__x86_64__) || defined(_M_X64)
#define STBI__X64_TARGET
#elif defined(__i386) || defined(_M_IX86)
#define STBI__X86_TARGET
#endif

#if defined(__GNUC__) && defined(STBI__X86_TARGET) && !defined(__SSE2__) && !defined(STBI_NO_SIMD)
// gcc doesn't support sse2 intrinsics unless you pass -msse2,
// which in turn silently turns on SSE2 global optimization.
// If we want to use SSE2 intrinsics, we need to turn on SSE2 awareness.
// This code does not turn on the optimization flags, which we don't want.
#define STBI_NO_SIMD
#endif

#if defined(__MINGW32__) && defined(STBI__X86_TARGET) && !defined(STBI_MINGW_ENABLE_SSE2) && !defined(STBI_NO_SIMD)
// Note that __MINGW32__ doesn't imply the target is 32-bit, so we have to check STBI__X86_TARGET
//
// 32-bit MinGW wants ESP to be 16-byte aligned whenever it calls a
// function decorated with align(16). Other 32-bit gccs keep ESP 4-byte
// aligned, so we need to align it ourselves. It is messy. MinGW 64-bit
// functions with align(16) arguments are 16-byte aligned on entry already.
//
// We detect this situation by checking for the presence of _mm_malloc,
// which is defined when you use -msse2.
//
// You can override this behavior by defining STBI_MINGW_ENABLE_SSE2,
// in which case you are responsible for aligning the stack yourself.
#define STBI_NO_SIMD
#endif

#if !defined(STBI_NO_SIMD) && (defined(STBI__X86_TARGET) || defined(STBI__X64_TARGET))
#define STBI_SSE2
#include <emmintrin.h>

#ifdef _MSC_VER

#if _MSC_VER >= 1400 // not VC6
#include <intrin.h>  // __cpuid
static int stbi__cpuid3(void)
{
    int info[4];
    __cpuid(info, 1);
    return info[3];
}
#else
static int stbi__cpuid3(void)
{
    int res;
    __asm {
      mov  eax,1
      cpuid
      mov  res,edx
    }
    return res;
}
#endif

#define STBI_SIMD_ALIGN(type, name) __declspec(align(16)) type name

#if !defined(STBI_NO_JPEG) && defined(STBI_SSE2)
static int stbi__sse2_available(void)
{
    int info3 = stbi__cpuid3();
    return (info3 & 0x04000000) != 0;
}
#endif

#else // _MSC_VER

#define STBI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))

#if !defined(STBI_NO_JPEG) && defined(STBI_SSE2)
static int stbi__sse2_available(void)
{
    // If we're even attempting to compile this on GCC/Clang, that means
    // -msse2 is passed, which means we know SSE2 is available.
    return 1;
}
#endif

#endif
#endif

// ARM NEON
#if defined(STBI_NO_SIMD) && defined(STBI_NEON)
#undef STBI_NEON
#endif

#ifdef STBI_NEON
#include <arm_neon.h>
// assume GCC or Clang on ARM targets
#define STBI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))
#endif

#ifndef STBI_SIMD_ALIGN
#define STBI_SIMD_ALIGN(type, name) type name
#endif

#ifndef STBI_MAX_DIMENSIONS
#define STBI_MAX_DIMENSIONS (1 << 24)
#endif

///////////////////////////////////////////////
//
//  stbi__context struct and start_xxx functions

// stbi__context structure is our basic context used by all images, so it
// contains all the IO context, plus some basic image information
typedef struct
{
    stbi__uint32 img_x, img_y;
    int img_n, img_out_n;

    stbi_io_callbacks io;
    void *io_user_data;

    int read_from_callbacks;
    int buflen;
    stbi_uc buffer_start[128];
    int callback_already_read;

    stbi_uc *img_buffer, *img_buffer_end;
    stbi_uc *img_buffer_original, *img_buffer_original_end;
} stbi__context;

static void stbi__refill_buffer(stbi__context *s);

// initialize a memory-decode context
static void stbi__start_mem(stbi__context *s, stbi_uc const *buffer, int len)
{
    s->io.read = NULL;
    s->read_from_callbacks = 0;
    s->callback_already_read = 0;
    s->img_buffer = s->img_buffer_original = (stbi_uc *)buffer;
    s->img_buffer_end = s->img_buffer_original_end = (stbi_uc *)buffer + len;
}

// initialize a callback-decode context
static void stbi__start_callbacks(stbi__context *s, stbi_io_callbacks *c, void *user)
{
    s->io = *c;
    s->io_user_data = user;
    s->buflen = sizeof(s->buffer_start);
    s->read_from_callbacks = 1;
    s->callback_already_read = 0;
    s->img_buffer = s->img_buffer_original = s->buffer_start;
    stbi__refill_buffer(s);
    s->img_buffer_original_end = s->img_buffer_end;
}

#ifndef STBI_NO_STDIO
// initialize a file-decode context
static int stbi__start_file(stbi__context *s, FILE *f)
{
    static stbi_io_callbacks callbacks =
        {
            stbi__stdio_read,
            stbi__stdio_skip,
            stbi__stdio_eof,
        };
    stbi__start_callbacks(s, &callbacks, (void *)f);
    return 1;
}
#endif

static void stbi__rewind(stbi__context *s)
{
    // conceptually rewind SHOULD rewind to the beginning of the stream,
    // but we just rewind to the beginning of the initial buffer, because
    // we only use it for brief backtracking searches
    s->img_buffer = s->img_buffer_original;
}

enum
{
    STBI_ORDER_RGB,
    STBI_ORDER_BGR
};

typedef struct
{
    int bits_per_channel;
    int num_channels;
    int channel_order;
} stbi__result_info;

#ifndef STBI_NO_JPEG
static stbi_uc *stbi__jpeg_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int stbi__jpeg_test(stbi__context *s);
static int stbi__jpeg_info_raw(stbi__context *s, int *x, int *y, int *comp);
static int stbi__jpeg_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PNG
static stbi_uc *stbi__png_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int stbi__png_test(stbi__context *s);
static int stbi__png_info_raw(stbi__context *s, int *x, int *y, int *comp);
static int stbi__png_info(stbi__context *s, int *x, int *y, int *comp);
static int stbi__png_is16(stbi__context *s);
#endif

#ifndef STBI_NO_BMP
static stbi_uc *stbi__bmp_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int stbi__bmp_test(stbi__context *s);
static int stbi__bmp_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_TGA
static stbi_uc *stbi__tga_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int stbi__tga_test(stbi__context *s);
static int stbi__tga_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PSD
static stbi_uc *stbi__psd_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri, int bpc);
static int stbi__psd_test(stbi__context *s);
static int stbi__psd_info(stbi__context *s, int *x, int *y, int *comp);
static int stbi__psd_is16(stbi__context *s);
#endif

#ifndef STBI_NO_HDR
static float *stbi__hdr_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int stbi__hdr_test(stbi__context *s);
static int stbi__hdr_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PIC
static stbi_uc *stbi__pic_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int stbi__pic_test(stbi__context *s);
static int stbi__pic_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PNM
static stbi_uc *stbi__pnm_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int stbi__pnm_test(stbi__context *s);
static int stbi__pnm_info(stbi__context *s, int *x, int *y, int *comp);
static int stbi__pnm_is16(stbi__context *s);
#endif

#ifndef STBI_NO_GIF
static stbi_uc *stbi__gif_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int stbi__gif_test(stbi__context *s);
static int stbi__gif_info_raw(stbi__context *s, int *x, int *y, int *comp);
static int stbi__gif_info(stbi__context *s, int *x, int *y, int *comp);
#endif

// this is not thread-safe
static const char *stbi__g_failure_reason;

STBIDEF const char *stbi_failure_reason(void)
{
    return stbi__g_failure_reason;
}

#ifndef STBI_NO_FAILURE_STRINGS
static int stbi__err(const char *str)
{
    stbi__g_failure_reason = str;
    return 0;
}
#endif

static void *stbi__malloc(size_t size)
{
    return STBI_MALLOC(size);
}

// stb_image uses a lot of functions that can overflow integers.
// This library wraps all math in safe operations to avoid it.
//
// unsigned int addition: add vs stbi__addsizes_valid
// unsigned int multiplication: mul vs stbi__mul2sizes_valid
//
// The complexity of these functions makes it harder to read.
// However, it's not possible to write a simple addition function that
// will not overflow, because the C standard does not specify what
// happens on overflow. The original code had a lot of additions.
//
// stbi__addsizes_valid is probably not correct, but it's probably good enough.

static int stbi__addsizes_valid(int a, int b)
{
    if (b < 0)
        return 0;
    // now 0 <= b
    return a <= INT_MAX - b;
}

// returns 1 if the product is valid, 0 on overflow.
// negative factors are considered invalid.
static int stbi__mul2sizes_valid(int a, int b)
{
    if (a < 0 || b < 0)
        return 0;
    if (b == 0)
        return 1; // 0 * anything is 0
    return a <= INT_MAX / b;
}

// returns 1 if the product is valid, 0 on overflow.
// negative factors are considered invalid.
static int stbi__mul3sizes_valid(int a, int b, int c)
{
    if (a < 0 || b < 0 || c < 0)
        return 0;
    if (b == 0 || c == 0)
        return 1; // 0 * anything is 0
    if (!stbi__mul2sizes_valid(a, b))
        return 0;
    return c <= INT_MAX / (a * b);
}

// returns 1 if the product is valid, 0 on overflow.
// negative factors are considered invalid.
static int stbi__mul4sizes_valid(int a, int b, int c, int d)
{
    if (a < 0 || b < 0 || c < 0 || d < 0)
        return 0;
    if (b == 0 || c == 0 || d == 0)
        return 1; // 0 * anything is 0
    if (!stbi__mul2sizes_valid(a, b))
        return 0;
    if (!stbi__mul2sizes_valid(c, d))
        return 0;
    return (a * b) <= INT_MAX / (c * d);
}

// returns 1 if the sum is valid, 0 on overflow.
// negative factors are considered invalid.
static int stbi__add3sizes_valid(int a, int b, int c)
{
    return stbi__addsizes_valid(a, b) && stbi__addsizes_valid(a + b, c);
}

// returns 1 if the sum is valid, 0 on overflow.
// negative factors are considered invalid.
static int stbi__add4sizes_valid(int a, int b, int c, int d)
{
    return stbi__addsizes_valid(a, b) && stbi__addsizes_valid(a + b, c) && stbi__addsizes_valid(a + b + c, d);
}

// this is thread-safe
static void stbi__err_clear(void)
{
    stbi__g_failure_reason = NULL;
}

#if !defined(STBI_NO_FAILURE_STRINGS)
#define stbi__errpf(x, y) ((void)(stbi__err(x)), (void)0)
#define stbi__errpuc(x, y) ((void)(stbi__err(x)), (void *)0)
#else
#define stbi__errpf(x, y) 0
#define stbi__errpuc(x, y) 0
#endif

STBIDEF void stbi_image_free(void *retval_from_stbi_load)
{
    STBI_FREE(retval_from_stbi_load);
}

#ifndef STBI_NO_ZLIB
// implementation of stbi_zlib_decode_malloc_guesssize_headerflag
// This is not a full zlib decoder, but it's enough for PNG and GIF.
// It does not support all possible zlib options.

#ifdef STBI_NO_CRC
#define stbi__crc32(a, b) 0
#else
static stbi__uint32 stbi__crc32(stbi__uint32 crc, const stbi_uc *buf, int len)
{
    static const stbi__uint32 crc_table[256] =
        {
            0x00000000,
            0x77073096,
            0xEE0E612C,
            0x990951BA,
            0x076DC419,
            0x706AF48F,
            0xE963A535,
            0x9E6495A3,
            0x0EDB8832,
            0x79DCB8A4,
            0xE0D5E91E,
            0x97D2D988,
            0x09B64C2B,
            0x7EB17CBD,
            0xE7B82D07,
            0x90BF1D91,
            0x1DB71064,
            0x6AB020F2,
            0xF3B97148,
            0x84BE41DE,
            0x1ADAD47D,
            0x6DDDE4EB,
            0xF4D4B551,
            0x83D385C7,
            0x136C9856,
            0x646BA8C0,
            0xFD62F97A,
            0x8A65C9EC,
            0x14015C4F,
            0x63066CD9,
            0xFA0F3D63,
            0x8D080DF5,
            0x3B6E20C8,
            0x4C69105E,
            0xD56041E4,
            0xA2677172,
            0x3C03E4D1,
            0x4B04D447,
            0xD20D85FD,
            0xA50AB56B,
            0x35B5A8FA,
            0x42B2986C,
            0xDBBBC9D6,
            0xACBCF940,
            0x32D86CE3,
            0x45DF5C75,
            0xDCD60DCF,
            0xABD13D59,
            0x26D930AC,
            0x51DE003A,
            0xC8D75180,
            0xBFD06116,
            0x21B4F4B5,
            0x56B3C423,
            0xCFBA9599,
            0xB8BDA50F,
            0x2802B89E,
            0x5F058808,
            0xC60CD9B2,
            0xB10BE924,
            0x2F6F7C87,
            0x58684C11,
            0xC1611DAB,
            0xB6662D3D,
            0x76DC4190,
            0x01DB7106,
            0x98D220BC,
            0xEFD5102A,
            0x71B18589,
            0x06B6B51F,
            0x9FBFE4A5,
            0xE8B8D433,
            0x7807C9A2,
            0x0F00F934,
            0x9609A88E,
            0xE10E9818,
            0x7F6A0DBB,
            0x086D3D2D,
            0x91646C97,
            0xE6635C01,
            0x6B6B51F4,
            0x1C6C6162,
            0x856530D8,
            0xF262004E,
            0x6C0695ED,
            0x1B01A57B,
            0x8208F4C1,
            0xF50FC457,
            0x65B0D9C6,
            0x12B7E950,
            0x8BBEB8EA,
            0xFCB9887C,
            0x62DD1DDF,
            0x15DA2D49,
            0x8CD37CF3,
            0xFBD44C65,
            0x4DB26158,
            0x3AB551CE,
            0xA3BC0074,
            0xD4BB30E2,
            0x4ADFA541,
            0x3DD895D7,
            0xA4D1C46D,
            0xD3D6F4FB,
            0x4369E96A,
            0x346ED9FC,
            0xAD678846,
            0xDA60B8D0,
            0x44042D73,
            0x33031DE5,
            0xAA0A4C5F,
            0xDD0D7CC9,
            0x5005713C,
            0x270241AA,
            0xBE0B1010,
            0xC90C2086,
            0x5768B525,
            0x206F85B3,
            0xB966D409,
            0xCE61E49F,
            0x5EDEF90E,
            0x29D9C998,
            0xB0D09822,
            0xC7D7A8B4,
            0x59B33D17,
            0x2EB40D81,
            0xB7BD5C3B,
            0xC0BA6CAD,
            0xEDB88320,
            0x9ABFB3B6,
            0x03B6E20C,
            0x74B1D29A,
            0xEAD54739,
            0x9DD277AF,
            0x04DB2615,
            0x73DC1683,
            0xE3630B12,
            0x94643B84,
            0x0D6D6A3E,
            0x7A6A5AA8,
            0xE40ECF0B,
            0x9309FF9D,
            0x0A00AE27,
            0x7D079EB1,
            0xF00F9344,
            0x8708A3D2,
            0x1E01F268,
            0x6906C2FE,
            0xF762575D,
            0x806567CB,
            0x196C3671,
            0x6E6B06E7,
            0xFED41B76,
            0x89D32BE0,
            0x10DA7A5A,
            0x67DD4ACC,
            0xF9B9DF6F,
            0x8EBEEFF9,
            0x17B7BE43,
            0x60B08ED5,
            0xD6D6A3E8,
            0xA1D1937E,
            0x38D8C2C4,
            0x4FDFF252,
            0xD1BB67F1,
            0xA6BC5767,
            0x3FB506DD,
            0x48B2364B,
            0xD80D2BDA,
            0xAF0A1B4C,
            0x36034AF6,
            0x41047A60,
            0xDF60EFC3,
            0xA867DF55,
            0x316E8EEF,
            0x4669BE79,
            0xCB61B38C,
            0xBC66831A,
            0x256FD2A0,
            0x5268E236,
            0xCC0C7795,
            0xBB0B4703,
            0x220216B9,
            0x5505262F,
            0xC5BA3BBE,
            0xB2BD0B28,
            0x2BB45A92,
            0x5CB36A04,
            0xC2D7FFA7,
            0xB5D0CF31,
            0x2CD99E8B,
            0x5BDEAE1D,
        };
    crc = ~crc;
    while (len--)
        crc = (crc >> 8) ^ crc_table[(crc ^ *buf++) & 0xFF];
    return ~crc;
}
#endif

// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
//    simple implementation
//      - all input must be provided in an in-memory buffer
//      - OK for tiny zlib data streams
//      - decoding builds backwards in output buffer and avoids bounds checks
//      - memory usage is proportional to output size
//    optimization future:
//      - move decompression switch inside loop to avoid extra branches->done
//      - look ahead for trivial blocks->done
//      - look ahead for short string matches->done
//      - allow big output buffer and check for overflow when detecting case 4
//      - allow direct writing to output buffer instead of caching indices

static int stbi__decompress(stbi_uc *out, int out_len, const stbi_uc *in, int in_len)
{
    // parameters for zlib decoder
    stbi__uint32 CMF, FLG;
    int CM, CINFO, FDICT;
    int i, type;
    stbi__uint32 adler32;

    if (in_len < 2)
        return stbi__err("zlib corrupt", "Corrupt zlib stream");
    CMF = in[0];
    FLG = in[1];
    CM = CMF & 15;
    CINFO = CMF >> 4;
    FDICT = (FLG >> 5) & 1;
    if (CM != 8 || CINFO > 7)
        return stbi__err("zlib corrupt", "Corrupt zlib stream");
    if ((CMF * 256 + FLG) % 31 != 0)
        return stbi__err("zlib corrupt", "Corrupt zlib stream");
    if (FDICT)
        return stbi__err("preset dictionary not supported", "Unsupported zlib format");
    adler32 = 0; // required by standard to be 1
    i = 0;
    do
    {
        type = (in[i + 2] >> 1) & 3;
        if (type == 0)
        { // stored block
            int len;
            if (i + 4 > in_len)
                return stbi__err("zlib corrupt", "Corrupt zlib stream");
            len = in[i + 3] | (in[i + 4] << 8);
            if (i + 5 + len > in_len)
                return stbi__err("zlib corrupt", "Corrupt zlib stream");
            if (out_len < len)
                return stbi__err("zlib out of memory", "Out of memory");
            memcpy(out, in + i + 5, len);
            out += len;
            out_len -= len;
            i += 5 + len;
        }
        else if (type == 1 || type == 2)
        { // compressed block
            // @TODO: implement
            return stbi__err("dynamic huffman coded block not supported", "Unsupported zlib format");
        }
        else
        {
            return stbi__err("zlib corrupt", "Corrupt zlib stream");
        }
    } while (!(in[i + 2] & 1));
    if (i + 6 > in_len)
        return stbi__err("zlib corrupt", "Corrupt zlib stream");
    adler32 = (in[i + 3] << 24) | (in[i + 4] << 16) | (in[i + 5] << 8) | in[i + 6];
    // @TODO: check adler32
    (void)adler32;
    return 1;
}

STBIDEF char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen)
{
    char *p = (char *)stbi__malloc(initial_size);
    if (p == NULL)
        return NULL;
    if (stbi__decompress((stbi_uc *)p, initial_size, (stbi_uc *)buffer, len))
    {
        if (outlen)
            *outlen = initial_size;
        return p;
    }
    else
    {
        STBI_FREE(p);
        return NULL;
    }
}

STBIDEF char *stbi_zlib_decode_malloc(const char *buffer, int len, int *outlen)
{
    return stbi_zlib_decode_malloc_guesssize(buffer, len, 16384, outlen);
}

STBIDEF char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header)
{
    if (parse_header)
    {
        return stbi_zlib_decode_malloc_guesssize(buffer, len, initial_size, outlen);
    }
    else
    {
        char *p = (char *)stbi__malloc(initial_size);
        if (p == NULL)
            return NULL;
        if (stbi__decompress((stbi_uc *)p, initial_size, (stbi_uc *)buffer - 2, len + 2))
        {
            if (outlen)
                *outlen = initial_size;
            return p;
        }
        else
        {
            STBI_FREE(p);
            return NULL;
        }
    }
}

STBIDEF int stbi_zlib_decode_buffer(char *obuffer, int olen, const char *ibuffer, int ilen)
{
    return stbi__decompress((stbi_uc *)obuffer, olen, (stbi_uc *)ibuffer, ilen);
}

STBIDEF char *stbi_zlib_decode_noheader_malloc(const char *buffer, int len, int *outlen)
{
    char *p = (char *)stbi__malloc(16384);
    if (p == NULL)
        return NULL;
    if (stbi__decompress((stbi_uc *)p, 16384, (stbi_uc *)buffer - 2, len + 2))
    {
        if (outlen)
            *outlen = 16384;
        return p;
    }
    else
    {
        STBI_FREE(p);
        return NULL;
    }
}

STBIDEF int stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen)
{
    return stbi__decompress((stbi_uc *)obuffer, olen, (stbi_uc *)ibuffer - 2, ilen + 2);
}
#endif

// stb_image_write - v1.16 - public domain - http://nothings.org/stb/stb_image_write.h
// writes out PNG/BMP/TGA/JPEG/HDR images from 8-bit-per-channel data
//
// QUICK NOTES:
//      stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
//      stbi_write_bmp(char const *filename, int w, int h, int comp, const void *data);
//      stbi_write_tga(char const *filename, int w, int h, int comp, const void *data);
//      stbi_write_jpg(char const *filename, int w, int h, int comp, const void *data, int quality);
//      stbi_write_hdr(char const *filename, int w, int h, int comp, const float *data);
//
//      stbi_write_png_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data, int stride_in_bytes);
//      stbi_write_bmp_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data);
//      stbi_write_tga_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data);
//      stbi_write_jpg_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data, int quality);
//      stbi_write_hdr_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const float *data);
//
//      See stbi_write_png_compression_level to specify PNG compression, see stbi_write_force_png_filter to force a filter,
//      see stbi_write_no_force_png_filter to disable force filter.

#ifndef STBI_WRITE_INCLUDE_STB_IMAGE_WRITE_H
#define STBI_WRITE_INCLUDE_STB_IMAGE_WRITE_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef STB_IMAGE_WRITE_STATIC
#define STBIWDEF static
#else
#define STBIWDEF extern
#endif

    typedef void stbi_write_func(void *context, void *data, int size);

    STBIWDEF int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
    STBIWDEF int stbi_write_bmp(char const *filename, int w, int h, int comp, const void *data);
    STBIWDEF int stbi_write_tga(char const *filename, int w, int h, int comp, const void *data);
    STBIWDEF int stbi_write_hdr(char const *filename, int w, int h, int comp, const float *data);
#ifndef STBI_NO_JPEG
    STBIWDEF int stbi_write_jpg(char const *filename, int w, int h, int comp, const void *data, int quality);
#endif

    STBIWDEF int stbi_write_png_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void *data, int stride_in_bytes);
    STBIWDEF int stbi_write_bmp_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void *data);
    STBIWDEF int stbi_write_tga_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void *data);
    STBIWDEF int stbi_write_hdr_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const float *data);
#ifndef STBI_NO_JPEG
    STBIWDEF int stbi_write_jpg_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void *data, int quality);
#endif

    STBIWDEF void stbi_write_png_compression_level(int level);
    STBIWDEF void stbi_write_force_png_filter(int filter);
    STBIWDEF void stbi_write_no_force_png_filter(void);

#ifdef __cplusplus
}
#endif

#endif // STBI_WRITE_INCLUDE_STB_IMAGE_WRITE_H

#ifdef STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#endif

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif // STBI_NO_STDIO

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(STBIW_MALLOC) && defined(STBIW_FREE) && defined(STBIW_REALLOC)
// ok
#elif !defined(STBIW_MALLOC) && !defined(STBIW_FREE) && !defined(STBIW_REALLOC)
// ok
#else
#error "Must define all or none of STBIW_MALLOC, STBIW_FREE, and STBIW_REALLOC."
#endif

#ifndef STBIW_MALLOC
#define STBIW_MALLOC(sz) malloc(sz)
#define STBIW_REALLOC(p, newsz) realloc(p, newsz)
#define STBIW_FREE(p) free(p)
#endif

static void *stbiw__malloc(size_t size)
{
    return STBIW_MALLOC(size);
}

static void stbiw__free(void *p)
{
    STBIW_FREE(p);
}

static void *stbiw__realloc(void *p, size_t new_size)
{
    return STBIW_REALLOC(p, new_size);
}

typedef struct
{
    stbi_write_func *func;
    void *context;
    unsigned char buffer[64];
    int buf_used;
} stbi__write_context;

// initialize a callback-based context
static void stbi__start_write_callbacks(stbi__write_context *s, stbi_write_func *f, void *c)
{
    s->func = f;
    s->context = c;
    s->buf_used = 0;
}

#ifndef STBI_NO_STDIO
static void stbi__stdio_write(void *context, void *data, int size)
{
    fwrite(data, 1, size, (FILE *)context);
}

static int stbi__start_write_file(stbi__write_context *s, const char *filename)
{
    FILE *f = fopen(filename, "wb");
    stbi__start_write_callbacks(s, stbi__stdio_write, (void *)f);
    return f != NULL;
}

static void stbi__end_write_file(stbi__write_context *s)
{
    fclose((FILE *)s->context);
}
#endif // !STBI_NO_STDIO

static void stbiw__putc(stbi__write_context *s, unsigned char c)
{
    if (s->buf_used == 64)
    {
        s->func(s->context, s->buffer, 64);
        s->buf_used = 0;
    }
    s->buffer[s->buf_used++] = c;
}

static void stbiw__flush(stbi__write_context *s)
{
    if (s->buf_used > 0)
    {
        s->func(s->context, s->buffer, s->buf_used);
        s->buf_used = 0;
    }
}

static void stbiw__write(stbi__write_context *s, const void *p, int n)
{
    stbiw__flush(s);
    s->func(s->context, (void *)p, n);
}

static void stbiw__put_be16(stbi__write_context *s, int val)
{
    stbiw__putc(s, (val >> 8) & 0xff);
    stbiw__putc(s, val & 0xff);
}

static void stbiw__put_be32(stbi__write_context *s, int val)
{
    stbiw__putc(s, (val >> 24) & 0xff);
    stbiw__putc(s, (val >> 16) & 0xff);
    stbiw__putc(s, (val >> 8) & 0xff);
    stbiw__putc(s, val & 0xff);
}

static void stbiw__put_le16(stbi__write_context *s, int val)
{
    stbiw__putc(s, val & 0xff);
    stbiw__putc(s, (val >> 8) & 0xff);
}

static void stbiw__put_le32(stbi__write_context *s, int val)
{
    stbiw__putc(s, val & 0xff);
    stbiw__putc(s, (val >> 8) & 0xff);
    stbiw__putc(s, (val >> 16) & 0xff);
    stbiw__putc(s, (val >> 24) & 0xff);
}

#ifndef STBI_NO_PNG
static unsigned char stbiw__paeth(int a, int b, int c)
{
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);
    if (pa <= pb && pa <= pc)
        return (unsigned char)a;
    if (pb <= pc)
        return (unsigned char)b;
    return (unsigned char)c;
}

static unsigned int stbiw__crc32(unsigned char *buffer, int len)
{
    static const unsigned int crc_table[256] =
        {
            0x00000000,
            0x77073096,
            0xEE0E612C,
            0x990951BA,
            0x076DC419,
            0x706AF48F,
            0xE963A535,
            0x9E6495A3,
            0x0EDB8832,
            0x79DCB8A4,
            0xE0D5E91E,
            0x97D2D988,
            0x09B64C2B,
            0x7EB17CBD,
            0xE7B82D07,
            0x90BF1D91,
            0x1DB71064,
            0x6AB020F2,
            0xF3B97148,
            0x84BE41DE,
            0x1ADAD47D,
            0x6DDDE4EB,
            0xF4D4B551,
            0x83D385C7,
            0x136C9856,
            0x646BA8C0,
            0xFD62F97A,
            0x8A65C9EC,
            0x14015C4F,
            0x63066CD9,
            0xFA0F3D63,
            0x8D080DF5,
            0x3B6E20C8,
            0x4C69105E,
            0xD56041E4,
            0xA2677172,
            0x3C03E4D1,
            0x4B04D447,
            0xD20D85FD,
            0xA50AB56B,
            0x35B5A8FA,
            0x42B2986C,
            0xDBBBC9D6,
            0xACBCF940,
            0x32D86CE3,
            0x45DF5C75,
            0xDCD60DCF,
            0xABD13D59,
            0x26D930AC,
            0x51DE003A,
            0xC8D75180,
            0xBFD06116,
            0x21B4F4B5,
            0x56B3C423,
            0xCFBA9599,
            0xB8BDA50F,
            0x2802B89E,
            0x5F058808,
            0xC60CD9B2,
            0xB10BE924,
            0x2F6F7C87,
            0x58684C11,
            0xC1611DAB,
            0xB6662D3D,
            0x76DC4190,
            0x01DB7106,
            0x98D220BC,
            0xEFD5102A,
            0x71B18589,
            0x06B6B51F,
            0x9FBFE4A5,
            0xE8B8D433,
            0x7807C9A2,
            0x0F00F934,
            0x9609A88E,
            0xE10E9818,
            0x7F6A0DBB,
            0x086D3D2D,
            0x91646C97,
            0xE6635C01,
            0x6B6B51F4,
            0x1C6C6162,
            0x856530D8,
            0xF262004E,
            0x6C0695ED,
            0x1B01A57B,
            0x8208F4C1,
            0xF50FC457,
            0x65B0D9C6,
            0x12B7E950,
            0x8BBEB8EA,
            0xFCB9887C,
            0x62DD1DDF,
            0x15DA2D49,
            0x8CD37CF3,
            0xFBD44C65,
            0x4DB26158,
            0x3AB551CE,
            0xA3BC0074,
            0xD4BB30E2,
            0x4ADFA541,
            0x3DD895D7,
            0xA4D1C46D,
            0xD3D6F4FB,
            0x4369E96A,
            0x346ED9FC,
            0xAD678846,
            0xDA60B8D0,
            0x44042D73,
            0x33031DE5,
            0xAA0A4C5F,
            0xDD0D7CC9,
            0x5005713C,
            0x270241AA,
            0xBE0B1010,
            0xC90C2086,
            0x5768B525,
            0x206F85B3,
            0xB966D409,
            0xCE61E49F,
            0x5EDEF90E,
            0x29D9C998,
            0xB0D09822,
            0xC7D7A8B4,
            0x59B33D17,
            0x2EB40D81,
            0xB7BD5C3B,
            0xC0BA6CAD,
            0xEDB88320,
            0x9ABFB3B6,
            0x03B6E20C,
            0x74B1D29A,
            0xEAD54739,
            0x9DD277AF,
            0x04DB2615,
            0x73DC1683,
            0xE3630B12,
            0x94643B84,
            0x0D6D6A3E,
            0x7A6A5AA8,
            0xE40ECF0B,
            0x9309FF9D,
            0x0A00AE27,
            0x7D079EB1,
            0xF00F9344,
            0x8708A3D2,
            0x1E01F268,
            0x6906C2FE,
            0xF762575D,
            0x806567CB,
            0x196C3671,
            0x6E6B06E7,
            0xFED41B76,
            0x89D32BE0,
            0x10DA7A5A,
            0x67DD4ACC,
            0xF9B9DF6F,
            0x8EBEEFF9,
            0x17B7BE43,
            0x60B08ED5,
            0xD6D6A3E8,
            0xA1D1937E,
            0x38D8C2C4,
            0x4FDFF252,
            0xD1BB67F1,
            0xA6BC5767,
            0x3FB506DD,
            0x48B2364B,
            0xD80D2BDA,
            0xAF0A1B4C,
            0x36034AF6,
            0x41047A60,
            0xDF60EFC3,
            0xA867DF55,
            0x316E8EEF,
            0x4669BE79,
            0xCB61B38C,
            0xBC66831A,
            0x256FD2A0,
            0x5268E236,
            0xCC0C7795,
            0xBB0B4703,
            0x220216B9,
            0x5505262F,
            0xC5BA3BBE,
            0xB2BD0B28,
            0x2BB45A92,
            0x5CB36A04,
            0xC2D7FFA7,
            0xB5D0CF31,
            0x2CD99E8B,
            0x5BDEAE1D,
        };
    unsigned int crc = ~0u;
    int i;
    for (i = 0; i < len; ++i)
        crc = (crc >> 8) ^ crc_table[buffer[i] ^ (crc & 0xff)];
    return ~crc;
}

#define stbiw__add(a, b) ((a) + (b))
#define stbiw__sub(a, b) ((a) - (b))
#define stbiw__mul(a, b) ((a) * (b))

// fast-way is faster to check than jpeg huffman, but slow way is slower
#define STBIW__ZFAST_BITS 9 // accelerate all cases in default tables
#define STBIW__ZFAST_MASK ((1 << STBIW__ZFAST_BITS) - 1)

typedef struct
{
    stbiw_uint16 fast[1 << STBIW__ZFAST_BITS];
    stbiw_uint16 firstcode[16];
    int maxcode[17];
    stbiw_uint16 firstsymbol[16];
    unsigned char size[288];
    stbiw_uint16 value[288];
} stbiw__zhuffman;

stbiw_inline static int stbiw__bitrev(int n, int b)
{
    int rev = 0;
    while (b--)
    {
        rev = (rev << 1) | (n & 1);
        n >>= 1;
    }
    return rev;
}

static int stbiw__zbuild_huffman(stbiw__zhuffman *z, const unsigned char *sizelist, int num)
{
    int i, k = 0;
    int code, next_code[16], sizes[17];

    // DEFLATE spec for generating codes
    memset(sizes, 0, sizeof(sizes));
    memset(z->fast, 0, sizeof(z->fast));
    for (i = 0; i < num; ++i)
        ++sizes[sizelist[i]];
    sizes[0] = 0;
    for (i = 1; i < 16; ++i)
        if (sizes[i] > (1 << i))
            return 0; // over-subscribed--impossible
    code = 0;
    for (i = 1; i < 16; ++i)
    {
        next_code[i] = code;
        z->firstcode[i] = (stbiw_uint16)code;
        z->firstsymbol[i] = (stbiw_uint16)k;
        code = (code + sizes[i]);
        if (sizes[i])
            if (code - 1 >= (1 << i))
                return 0;
        z->maxcode[i] = code << (16 - i); // preshift for inner loop
        code <<= 1;
        k += sizes[i];
    }
    z->maxcode[16] = 0x10000; // sentinel
    for (i = 0; i < num; ++i)
    {
        int s = sizelist[i];
        if (s)
        {
            int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
            stbiw_uint16 fastv = (stbiw_uint16)((s << 9) | i);
            z->size[c] = (unsigned char)s;
            z->value[c] = (stbiw_uint16)i;
            if (s <= STBIW__ZFAST_BITS)
            {
                int j = stbiw__bitrev(next_code[s], s);
                while (j < (1 << STBIW__ZFAST_BITS))
                {
                    z->fast[j] = fastv;
                    j += (1 << s);
                }
            }
            ++next_code[s];
        }
    }
    return 1;
}

// zlib-style huffman encoding
// take 16 bits of input at once
#define stbiw__zsend(s, c) (stbiw__zout(s, (c), 1), (c))
#define stbiw__zsend2(s, c) (stbiw__zout(s, (c), 2), (c))
#define stbiw__zsend3(s, c) (stbiw__zout(s, (c), 3), (c))
#define stbiw__zsend4(s, c) (stbiw__zout(s, (c), 4), (c))

typedef struct
{
    stbi__write_context *z;
    unsigned char *out;
    int cur, bitcount;
    stbiw_uint32 code_buffer;
} stbiw__zbuf;

static void stbiw__zout(stbiw__zbuf *z, int val, int bits)
{
    z->code_buffer |= (val << z->bitcount);
    z->bitcount += bits;
    while (z->bitcount >= 8)
    {
        z->out[z->cur++] = (unsigned char)(z->code_buffer & 0xff);
        z->code_buffer >>= 8;
        z->bitcount -= 8;
    }
}

static void stbiw__zflush(stbiw__zbuf *z)
{
    if (z->bitcount)
        stbiw__zout(z, 0, 8 - z->bitcount);
    if (z->cur)
    {
        stbiw__write(z->z, z->out, z->cur);
        z->cur = 0;
    }
}

static int stbiw__zlib_huffman_block(unsigned char *out, unsigned char *data, int bit_depth, int w, int h, int comp)
{
    // stub
    return 0;
}

static int stbiw__zlib_deflate(stbi__write_context *s, unsigned char *data, int data_len, int comp)
{
    // stub
    return 0;
}

static unsigned int stbiw__adler32(unsigned char *buffer, int len)
{
    const unsigned int ADLER_MOD = 65521;
    unsigned int s1 = 1, s2 = 0;
    int i;
    for (i = 0; i < len; ++i)
    {
        s1 = (s1 + buffer[i]) % ADLER_MOD;
        s2 = (s2 + s1) % ADLER_MOD;
    }
    return (s2 << 16) + s1;
}

static void stbiw__write_zlib_header(stbi__write_context *s, int cmf, int flg)
{
    stbiw__putc(s, cmf);
    stbiw__putc(s, flg);
}

static void stbiw__write_zlib_footer(stbi__write_context *s, unsigned int adler)
{
    stbiw__put_be32(s, adler);
}

STBIWDEF void stbi_write_png_compression_level(int level)
{
    // stub
}

STBIWDEF void stbi_write_force_png_filter(int filter)
{
    // stub
}

STBIWDEF void stbi_write_no_force_png_filter(void)
{
    // stub
}

static int stbi_write_png_core(stbi__write_context *s, int x, int y, int comp, const void *data, int stride_in_bytes)
{
    // stub
    return 0;
}

STBIWDEF int stbi_write_png_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data, int stride_in_bytes)
{
    stbi__write_context s;
    stbi__start_write_callbacks(&s, func, context);
    return stbi_write_png_core(&s, x, y, comp, data, stride_in_bytes);
}

#ifndef STBI_NO_STDIO
STBIWDEF int stbi_write_png(char const *filename, int x, int y, int comp, const void *data, int stride_in_bytes)
{
    stbi__write_context s;
    if (stbi__start_write_file(&s, filename))
    {
        int r = stbi_write_png_core(&s, x, y, comp, data, stride_in_bytes);
        stbi__end_write_file(&s);
        return r;
    }
    else
        return 0;
}
#endif // !STBI_NO_STDIO
#endif // !STBI_NO_PNG

#ifndef STBI_NO_BMP
static int stbi_write_bmp_core(stbi__write_context *s, int x, int y, int comp, const void *data)
{
    int pad = (-x * 3) & 3;
    int filesize = 54 + 3 * x * y + pad * y;
    stbiw__putc(s, 'B');
    stbiw__putc(s, 'M');
    stbiw__put_le32(s, filesize);
    stbiw__put_le16(s, 0);
    stbiw__put_le16(s, 0);
    stbiw__put_le32(s, 54);
    stbiw__put_le32(s, 40);
    stbiw__put_le32(s, x);
    stbiw__put_le32(s, y);
    stbiw__put_le16(s, 1);
    stbiw__put_le16(s, 24);
    stbiw__put_le32(s, 0);
    stbiw__put_le32(s, 3 * x * y + pad * y);
    stbiw__put_le32(s, 0);
    stbiw__put_le32(s, 0);
    stbiw__put_le32(s, 0);
    stbiw__put_le32(s, 0);
    {
        int i, j, c;
        const unsigned char *p = (const unsigned char *)data;
        for (i = y - 1; i >= 0; --i)
        {
            for (j = 0; j < x; ++j)
            {
                stbiw__putc(s, p[i * x * comp + j * comp + 2]);
                stbiw__putc(s, p[i * x * comp + j * comp + 1]);
                stbiw__putc(s, p[i * x * comp + j * comp + 0]);
            }
            for (j = 0; j < pad; ++j)
                stbiw__putc(s, 0);
        }
    }
    return 1;
}

STBIWDEF int stbi_write_bmp_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data)
{
    stbi__write_context s;
    stbi__start_write_callbacks(&s, func, context);
    return stbi_write_bmp_core(&s, x, y, comp, data);
}

#ifndef STBI_NO_STDIO
STBIWDEF int stbi_write_bmp(char const *filename, int x, int y, int comp, const void *data)
{
    stbi__write_context s;
    if (stbi__start_write_file(&s, filename))
    {
        int r = stbi_write_bmp_core(&s, x, y, comp, data);
        stbi__end_write_file(&s);
        return r;
    }
    else
        return 0;
}
#endif // !STBI_NO_STDIO
#endif // !STBI_NO_BMP

#ifndef STBI_NO_TGA
static int stbi_write_tga_core(stbi__write_context *s, int x, int y, int comp, void *data)
{
    int i, j;
    unsigned char *p = (unsigned char *)data;
