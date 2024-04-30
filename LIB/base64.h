/**
 * @file base64.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-01-06
 * @last modified 2024-01-06
 *
 * @copyright Copyright (c) 2024 Liu Yuanlin Personal.
 *
 */
#ifndef BASE64_H
#define BASE64_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/**
 * @brief Wrapper function to encode a plain string of given length. Output is written
 * to *out without trailing zero. Output length in bytes is written to *outlen.
 * The buffer in `out` has been allocated by the caller and is at least 4/3 the
 * size of the input.
 * @param src
 * @param srclen
 * @param out
 * @param outlen set to NULL if you don't need it.
 * @param out_buf_capacity base64 encoded string buffer capacity, must be at least 4/3 + 1(save '\0') of input string length.
 * @return int 0 success, 1 output buffer is too small, 2 invalid argument.
 */
int base64_encode(const uint8_t *restrict src, size_t srclen, char *restrict out, size_t out_buf_capacity, size_t *outlen);

/**
 * @brief Wrapper function to decode a plain string of given length. Output is written
 * to *out without trailing zero. Output length in bytes is written to *outlen.
 * The buffer in `out` has been allocated by the caller and is at least 3/4 the
 * size of the input.This function can using the base64 string contain '\n' or
 * '\r' or '\t' or ' ' as input.
 *
 * @param src
 * @param out
 * @param out_buf_capacity base64 encoded string buffer capacity, must be at least 3/4 + 1(save '\0') of input string length.
 * @param outlen set to NULL if you don't need it.
 * @return int 0 success, 1 output buffer is too small, 2 invalid argument.
 */
int base64_decode(const char *restrict src, uint8_t *restrict out, size_t out_buf_capacity, size_t *outlen);
#ifdef __cplusplus
}
#endif
#endif //! BASE64_H
