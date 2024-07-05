#pragma once
/*
TurboJson string utilities.

BSD 3-Clause License

Copyright (c) 2024, Julien Perrier-cornet

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <cstdint>
#include <memory.h>
#include <string.h>


#ifdef AVX2
#if _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

static inline void aligned_memcpy(void* dst, void* src, size_t sz)
{
    uint8_t* source = (uint8_t*) src;
    uint8_t* destination = (uint8_t*) dst;
    uint8_t* end = (uint8_t*) src + (sz & 0xFFFFFFE0);
    while (source < end)
    {
        _mm256_store_si256( (__m256i*) destination, _mm256_load_si256((__m256i*) source) );
        source += 32;
        destination += 32;
    }
    end += (sz & 0x1F); // Deal with non-power-of-32 sizes :)
    while (source < end)
    {
        *destination = *source;
        source++;
        destination++;
    }
}

static inline void aligned_memset(void* dst, uint32_t elem, size_t sz)
{
    uint8_t* start = (uint8_t*) dst;
    uint8_t* end = ((uint8_t*) dst) + (sz & 0xFFFFFFE0);
    __m256i element = _mm256_set1_epi32( elem );
    while (start < end)
    {
        _mm256_store_si256( (__m256i*) start, element );
        start += 32;
    }
    end += (sz & 0x1F); // Deal with non-power-of-32 sizes :)
    while (start < end)
    {
        *start = elem;
        start++;
    }
}
#else
static inline void aligned_memcpy(void* dst, void* src, size_t sz)
{
    memcpy(dst, src, sz);
}

static inline void aligned_memset(void* dst, uint32_t elem, size_t sz)
{
    memset(dst, elem, sz);
}
#endif

