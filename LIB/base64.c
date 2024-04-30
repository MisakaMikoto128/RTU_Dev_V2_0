/**
 * @file base64.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-01-06
 * @last modified 2024-01-06
 *
 * @copyright Copyright (c) 2024 Liu Yuanlin Personal.
 *
 */
#include "base64.h"
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base64_encode(const uint8_t *restrict src, size_t srclen, char *restrict out, size_t out_buf_capacity, size_t *outlen)
{
    if (src == NULL || out == NULL || srclen == 0) {
        return 1; // Failure due to invalid arguments
    }

    size_t outlen_ = 4 * ((srclen + 2) / 3);   // Calculate the length of encoded string
    if (outlen_ >= out_buf_capacity) return 1; // Output buffer is too small
    if (outlen != NULL) {
        *outlen = outlen_;
    }

    size_t j = 0;
    for (size_t i = 0; i < srclen;) {
        uint32_t octet_a = i < srclen ? (uint8_t)src[i++] : 0;
        uint32_t octet_b = i < srclen ? (uint8_t)src[i++] : 0;
        uint32_t octet_c = i < srclen ? (uint8_t)src[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        out[j++] = base64_table[(triple >> 18) & 0x3F];
        out[j++] = base64_table[(triple >> 12) & 0x3F];
        out[j++] = base64_table[(triple >> 6) & 0x3F];
        out[j++] = base64_table[triple & 0x3F];
    }

    // Add padding
    const int mod_table[] = {0, 2, 1};
    size_t padding        = mod_table[srclen % 3];
    for (size_t i = 0; i < padding; i++) {
        out[outlen_ - 1 - i] = '=';
    }
    out[outlen_] = '\0';
    return 0; // Success
}

#if __STDC_VERSION__ >= 199901L
#define SUPPORTS_C99
#endif

#ifdef SUPPORTS_C99
// 定义Base64解码表，将Base64编码字符映射为相应的值
static const int base64_dec_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0-15: 无效字符
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 16-31: 无效字符
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, // 32-47: 无效字符, '+' 解码为 62, '/' 解码为 63
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, // 48-63: '0'-'9' 解码为 0-9, 以及 'A'-'Z' 解码为 52-61
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,           // 64-79: 'a'-'z' 解码为 0-25
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, // 80-95: 'A'-'Z' 解码为 26-51
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 96-111: 'a'-'z' 解码为 27-51
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, // 112-127: '0'-'9' 解码为 41-51, 其他字符无效
    // 剩余字符都被标记为无效字符，用 -1 表示
    // 这些字符包括特殊字符和非Base64字符
};
#else
// 定义Base64解码表，将Base64编码字符映射为相应的值
static const int base64_dec_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0-15: 无效字符
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 16-31: 无效字符
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, // 32-47: 无效字符, '+' 解码为 62, '/' 解码为 63
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, // 48-63: '0'-'9' 解码为 0-9, 以及 'A'-'Z' 解码为 52-61
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,           // 64-79: 'a'-'z' 解码为 0-25
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, // 80-95: 'A'-'Z' 解码为 26-51
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 96-111: 'a'-'z' 解码为 27-51
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, // 112-127: '0'-'9' 解码为 41-51, 其他字符无效
    // 剩余字符都被标记为无效字符，用 -1 表示
    // 这些字符包括特殊字符和非Base64字符
};
#endif

int base64_decode(const char *restrict src, uint8_t *restrict out, size_t out_buf_capacity, size_t *outlen)
{
    if (src == NULL || out == NULL) {
        return 1; // Failure due to invalid arguments
    }

    size_t srclen                    = 0;
    size_t srclen_without_whitespace = 0;
    // Get Base64 string length without '\n' , '\r', '\t', ' '
    const char *p_src                 = src;
    const char *p_last_non_whitespace = NULL;
    while (*p_src != '\0') {
        if (*p_src != '\n' && *p_src != '\r' && *p_src != '\t' && *p_src != ' ') {
            srclen_without_whitespace++;
            p_last_non_whitespace = p_src;
        }
        p_src++;
        srclen++;
    }

    if (srclen_without_whitespace % 4 != 0 || srclen_without_whitespace == 0) return 2; // Invalid input size

    size_t outlen_ = srclen_without_whitespace / 4 * 3;

    if (*p_last_non_whitespace == '=') (outlen_)--;
    if (*(p_last_non_whitespace - 1) == '=') (outlen_)--;
    if (outlen_ >= out_buf_capacity) return 1; // Output buffer is too small
    if (outlen != NULL) {
        *outlen = outlen_;
    }

    size_t j = 0;
    for (size_t i = 0; i < srclen;) {
        uint32_t sextet_a = 0, sextet_b = 0, sextet_c = 0, sextet_d = 0;

        while (src[i] == '\n' || src[i] == '\r' || src[i] == '\t' || src[i] == ' ') { i++; }
        src[i] == '=' ? i++ : (sextet_a = base64_dec_table[src[i++]]);

        while (src[i] == '\n' || src[i] == '\r' || src[i] == '\t' || src[i] == ' ') { i++; }
        src[i] == '=' ? i++ : (sextet_b = base64_dec_table[src[i++]]);

        while (src[i] == '\n' || src[i] == '\r' || src[i] == '\t' || src[i] == ' ') { i++; }
        src[i] == '=' ? i++ : (sextet_c = base64_dec_table[src[i++]]);

        while (src[i] == '\n' || src[i] == '\r' || src[i] == '\t' || src[i] == ' ') { i++; }
        src[i] == '=' ? i++ : (sextet_d = base64_dec_table[src[i++]]);

        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;

        if (j < outlen_) out[j++] = (triple >> 16) & 0xFF;
        if (j < outlen_) out[j++] = (triple >> 8) & 0xFF;
        if (j < outlen_) out[j++] = triple & 0xFF;
    }

    return 0; // Success
}

void base64_test(const char *test_string)
{
    char encoded[1024];
    char decoded[1024];
    memset(encoded, 0, sizeof(encoded));
    memset(decoded, 0, sizeof(decoded));
    size_t outlen;

    printf("Original string: %s\n", test_string);

    // Test encoding
    if (base64_encode((uint8_t *)test_string, strlen(test_string), encoded, sizeof(encoded), &outlen) == 0) {
        printf("Encoded string: %s\n", encoded);
    } else {
        printf("Encoding failed\n");
    }

    // Test decoding
    if (base64_decode(encoded, (uint8_t *)decoded, sizeof(decoded), &outlen) == 0) {
        printf("Decoded string: %s\n", decoded);
    } else {
        printf("Decoding failed\n");
    }

    printf("\n");
}