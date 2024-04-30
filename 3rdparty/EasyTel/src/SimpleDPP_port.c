/**
 * @file SimpleDPP_port.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-09-28
 * @last modified 2023-09-28
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#include "SimpleDPP_port.h"
#include "HDL_CPU_Time.h"
#include <stdlib.h>
uint32_t SimpleDPP_getMsTick(void)
{
    // TODO:
    uint32_t msTimestamp = 0;
    msTimestamp = HDL_CPU_Time_GetTick();
    return msTimestamp;
}

void *sdp_malloc(unsigned int nbytes)
{
    return malloc(nbytes);
}

void sdp_free(void *ptr)
{
    free(ptr);
}

#if SDP_MEM_WATCH_EN
static unsigned int sdp_max_mem; /* Maximum memory used*/
static unsigned int sdp_cur_mem; /* Currently used memory*/
#endif

#if SDP_MEM_WATCH_EN
void *sdp_core_malloc(unsigned int nbytes)
{
    if (nbytes + sdp_cur_mem > SDP_MEM_LIMIT_SIZE)
    { // The maximum memory limit has been exceeded.
        return NULL;
    }
    unsigned long *mem_info = (unsigned long *)sdp_malloc(nbytes + sizeof(unsigned long));
    *mem_info = nbytes;
    sdp_cur_mem += nbytes;         // Statistics of current memory usage.
    if (sdp_cur_mem > sdp_max_mem) // Record maximum memory usage.
        sdp_max_mem = sdp_cur_mem;
    return mem_info + 1;
}

void sdp_core_free(void *ptr)
{
    unsigned long *mem_info = (unsigned long *)ptr;
    unsigned long nbyte;
    if (ptr != NULL)
    {
        mem_info--;
        nbyte = *mem_info;
        sdp_cur_mem -= nbyte;
        sdp_free(mem_info);
    }
}

/**
 * @brief Get the maximum memory usage.
 */
unsigned int sdp_max_used_memory(void)
{
    return sdp_max_mem;
}

/**
 * @brief Get current memory usage.
 */
unsigned int sdp_cur_used_memory(void)
{
    return sdp_cur_mem;
}

#else

void *sdp_core_malloc(unsigned int nbytes)
{
    return sdp_malloc(nbytes);
}

void sdp_core_free(void *ptr)
{
    sdp_free(ptr);
}

#endif