#ifndef TM_CORE_TM_FORMAT_H_
#define TM_CORE_TM_FORMAT_H_

#include "tm_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Number of decimal places of a fixed-point value: val 12345 with TM_DECIMALS_2 is 123.45.
typedef enum {
    TM_DECIMALS_0 = 0x00,
    TM_DECIMALS_1 = 0x01,
    TM_DECIMALS_2 = 0x02,
    TM_DECIMALS_3 = 0x03,
} tm_decimals;

/*
 * Number to text. All of them write at most `len` bytes including the
 * terminating NUL, truncating if needed, and return `buf`.
 */

// 12345 -> "12345", -5 -> "-5".
char* tm_format_int(char* buf, size_t len, int32_t val);

// Fixed number of decimals: 12345, TM_DECIMALS_2 -> "123.45"; 12340 -> "123.40".
char* tm_format_fixed(char* buf, size_t len, int32_t val, tm_decimals decimals);

// Shortest form ("%g"): 12340, TM_DECIMALS_2 -> "123.4"; 12300 -> "123".
char* tm_format_general(char* buf, size_t len, int32_t val, tm_decimals decimals);

#ifdef __cplusplus
}
#endif

#endif
