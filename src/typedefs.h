#ifndef TYPEDEFS_H
#define TYPEDEFS_H


#ifdef __cplusplus
#include <cstdint>
#define _STD_PREFIX std::
#else
#include <stdint.h>
#define _STD_PREFIX
#endif

typedef _STD_PREFIX int8_t           s8;
typedef _STD_PREFIX uint8_t          u8;
typedef _STD_PREFIX int16_t          s16;
typedef _STD_PREFIX uint16_t         u16;
typedef _STD_PREFIX int32_t          s32;
typedef _STD_PREFIX uint32_t         u32;
typedef _STD_PREFIX int64_t          s64;
typedef _STD_PREFIX uint64_t         u64;


#if defined(__cplusplus) && __cplusplus > 202002L

#include <stdfloat>

typedef std::float16_t               f16;
typedef std::float32_t               f32;
typedef std::float64_t               f64;
typedef std::float128_t             f128;

typedef std::bfloat16_t             bf16;

#else

typedef float                        f32;
typedef double                       f64;

#endif
#endif