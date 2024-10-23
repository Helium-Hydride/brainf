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

typedef float                        f32;
typedef double                       f64;

#endif