#pragma once

/*
TurboJson general API include file.

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


struct JsonContext {
    uint8_t *jsonbuffer;
    uint32_t jsonbufferSize;
    uint32_t jsonbufferMax;
    uint32_t *dom;
    uint32_t domIdx;
    uint32_t domSz;
    uint32_t *values;
    uint32_t valuesIdx;
    uint32_t valuesSz;
    uint8_t *jsonout;
    uint32_t jsonoutIdx;
    uint32_t jsonoutMax;
};


#if defined (__cplusplus)
extern "C" {
#endif

    struct JsonContext* turbojson_allocateContext();
    void turbojson_freeContext( struct JsonContext* ctx );

    void turbojson_parsefile( struct JsonContext* ctx, const char* jsonfilename );
    void turbojson_parsebuffer( struct JsonContext* ctx, uint8_t* jsonbuffer, uint32_t size, uint32_t allocsize );

    void turbojson_stringify( struct JsonContext* ctx );
    void turbojson_pretty( struct JsonContext* ctx, bool spaces, uint32_t numberSpaces, bool linereturn=true );

    void turbojson_writefile( struct JsonContext* ctx, const char* jsonfilename );

#if defined (__cplusplus)
}
#endif


