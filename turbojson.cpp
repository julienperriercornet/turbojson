/*
TurboJson implementation.

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


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>


#include "turbojson.h"
#include "platform.h"


extern "C" struct JsonContext* turbojson_allocateContext()
{
    struct JsonContext* context;

    context = (struct JsonContext*) align_alloc( MAX_CACHE_LINE_SIZE, sizeof(struct JsonContext) );

    if (context != nullptr)
    {
        context->jsonbuffer = nullptr;
        context->jsonbufferSize = 0;
        context->jsonbufferMax = 0;
        context->dom = nullptr;
        context->domIdx = 0;
        context->domSz = 0;
        context->values = nullptr;
        context->valuesIdx = 0;
        context->valuesSz = 0;
        context->jsonout = nullptr;
        context->jsonoutIdx = 0;
        context->jsonoutMax = 0;
    }

    return context;
}


extern "C" void turbojson_freeContext( struct JsonContext* ctx )
{
    if (ctx->jsonbuffer) align_free(ctx->jsonbuffer);
    if (ctx->dom) align_free(ctx->dom);
    if (ctx->values) align_free(ctx->values);
    if (ctx->jsonout) align_free(ctx->jsonout);
    align_free(ctx);
}


#define TURBOJSON_DOM_OBJECT 1
#define TURBOJSON_DOM_STRING 2
#define TURBOJSON_DOM_REAL 3
#define TURBOJSON_DOM_ARRAY 4
#define TURBOJSON_DOM_MEMBER 5
#define TURBOJSON_DOM_ARRAY_ELEMENT 6


#define turbojson_memcpy8( A, B ) *((uint64_t*) (A)) = *((const uint64_t*) (B))


static void turbojson_memcpy( uint8_t* dst, const uint8_t* src, const uint8_t* srcend )
{
    do {
        turbojson_memcpy8( dst, src );
        src += 8;
        dst += 8;
    } while (src < srcend) ;
}


extern "C" void turbojson_parsefile( struct JsonContext* ctx, const char* jsonfilename )
{
    FILE* in = fopen( jsonfilename, "rb" );

    if (in)
    {
        fseek( in, 0, SEEK_END );
        size_t filesize = ftell( in );
        fseek( in, 0, SEEK_SET );

        if (filesize > 0)
        {
            size_t allocfilesize = filesize + (filesize/2);
            uint8_t* buffer = (uint8_t*) align_alloc( MAX_CACHE_LINE_SIZE, allocfilesize );

            if (buffer != nullptr)
            {
                size_t readsize = fread( buffer, 1, filesize, in );

                if (readsize == filesize)
                {
                    turbojson_parsebuffer( ctx, buffer, filesize, allocfilesize );
                }
            }
        }

        fclose( in );
    }
}


static inline void skipSpaces( uint8_t* buffer, uint32_t *indice, uint32_t size )
{
    uint32_t i = *indice;
    while ((i < size) && (buffer[i] == ' ' || buffer[i] == '\t' || buffer[i] == '\n' || buffer[i] == '\r')) i++;
    *indice = i;
}


static uint32_t parseChildElement( uint8_t* buffer, uint32_t* indice, uint32_t size, uint32_t* dom, uint32_t* domIdx, uint32_t* domSz );


static uint32_t parseDouble( uint8_t* buffer, uint32_t* indice, uint32_t size, uint32_t* dom, uint32_t* domIdx, uint32_t* domSz )
{
    uint32_t i = *indice;
    uint32_t j = *domIdx;
    uint32_t dSz = *domSz;
    uint32_t oIdx = j;

    j += 3;
    dom[oIdx] = TURBOJSON_DOM_REAL;
    dom[oIdx+1] = i; // The indice of the real in UTF-8

    while ((i < size) && ((buffer[i] >= '0' && buffer[i] <= '9') || (buffer[i] == '.') || (buffer[i] == '-'))) i++;

    dom[oIdx+2] = i; // The indice of the end of the real string

    *indice = i;
    *domIdx = j;
    *domSz = dSz;

    return oIdx;
}


static uint32_t parseString( uint8_t* buffer, uint32_t* indice, uint32_t size, uint32_t* dom, uint32_t* domIdx, uint32_t* domSz )
{
    uint32_t i = *indice;
    uint32_t j = *domIdx;
    uint32_t dSz = *domSz;
    uint32_t oIdx = j;

    assert( buffer[i] == '"' );
    i++;

    j += 3;
    dom[oIdx] = TURBOJSON_DOM_STRING;
    dom[oIdx+1] = i; // The indice of the string

    while ( (i < size) && buffer[i] != '"' ) i++;

    dom[oIdx+2] = i; // The indice of the end of the string

    if (i < size) i++;

    *indice = i;
    *domIdx = j;
    *domSz = dSz;

    return oIdx;
}


static uint32_t parseObjectMember( uint8_t* buffer, uint32_t* indice, uint32_t size, uint32_t* dom, uint32_t* domIdx, uint32_t* domSz )
{
    uint32_t i = *indice;
    uint32_t j = *domIdx;
    uint32_t dSz = *domSz;
    uint32_t oIdx = 0xFFFFFFFF;

    assert( buffer[i] == '"' );
    i++;

    oIdx = j;
    j += 5;
    dom[oIdx] = TURBOJSON_DOM_MEMBER;
    dom[oIdx+1] = i; // The indice of the id string
    dom[oIdx+3] = 0xFFFFFFFF; // Child index
    dom[oIdx+4] = 0xFFFFFFFF; // The next member

    while ( (i < size) && buffer[i] != '"' ) i++;

    if (i < size)
    {
        dom[oIdx+2] = i++;
        skipSpaces( buffer, &i, size );
        assert( buffer[i] == ':' );
        i++;
        dom[oIdx+2] = parseChildElement( buffer, &i, size, dom, &j, &dSz );
    }

    *indice = i;
    *domIdx = j;
    *domSz = dSz;

    return oIdx;
}


static uint32_t parseObject( uint8_t* buffer, uint32_t* indice, uint32_t size, uint32_t* dom, uint32_t* domIdx, uint32_t* domSz )
{
    uint32_t i = *indice;
    uint32_t j = *domIdx;
    uint32_t dSz = *domSz;
    uint32_t oIdx = 0xFFFFFFFF;

    assert( buffer[i] == '{' );
    i++;

    oIdx = j;
    j += 2;
    dom[oIdx] = TURBOJSON_DOM_OBJECT;
    dom[oIdx+1] = 0xFFFFFFFF; // The list of members is a linked list whose last element points to -1

    uint32_t prevMemberIdx = 0xFFFFFFFF;

    do {
        skipSpaces( buffer, &i, size );

        uint32_t memberIdx = parseObjectMember( buffer, &i, size, dom, &j, &dSz );

        if (dom[oIdx+1] == 0xFFFFFFFF) dom[oIdx+1] = memberIdx;
        else dom[prevMemberIdx+4] = memberIdx;

        prevMemberIdx = memberIdx;

        skipSpaces( buffer, &i, size );

        assert( buffer[i] == ',' || buffer[i] == '}' );
        if (buffer[i] == ',') i++;

    } while ( (i < size) && buffer[i] != '}' );

    if (i < size) i++;
    //if (prevMemberIdx != 0xFFFFFFFF) dom[prevMemberIdx+4] = 0xFFFFFFFF;

    *indice = i;
    *domIdx = j;
    *domSz = dSz;

    return oIdx;
}


static uint32_t parseArrayElement( uint8_t* buffer, uint32_t* indice, uint32_t size, uint32_t* dom, uint32_t* domIdx, uint32_t* domSz )
{
    uint32_t i = *indice;
    uint32_t j = *domIdx;
    uint32_t dSz = *domSz;
    uint32_t oIdx = 0xFFFFFFFF;

    oIdx = j;
    j += 3;
    dom[oIdx] = TURBOJSON_DOM_ARRAY_ELEMENT;
    dom[oIdx+1] = parseChildElement( buffer, &i, size, dom, &j, &dSz );
    dom[oIdx+2] = 0xFFFFFFFF; // The next member

    *indice = i;
    *domIdx = j;
    *domSz = dSz;

    return oIdx;
}


static uint32_t parseArray( uint8_t* buffer, uint32_t* indice, uint32_t size, uint32_t* dom, uint32_t* domIdx, uint32_t* domSz )
{
    uint32_t i = *indice;
    uint32_t j = *domIdx;
    uint32_t dSz = *domSz;
    uint32_t oIdx = 0xFFFFFFFF;

    assert( buffer[i] == '[' );
    i++;

    oIdx = j;
    j += 2;
    dom[oIdx] = TURBOJSON_DOM_ARRAY;
    dom[oIdx+1] = 0xFFFFFFFF; // The elements of the array are stored in a linked list
    uint32_t prevElementIdx = 0xFFFFFFFF;

    do {
        uint32_t memberIdx = parseArrayElement( buffer, &i, size, dom, &j, &dSz );

        if (dom[oIdx+1] == 0xFFFFFFFF) dom[oIdx+1] = memberIdx;
        else dom[prevElementIdx+2] = memberIdx;

        prevElementIdx = memberIdx;

        skipSpaces( buffer, &i, size );

        assert( buffer[i] == ',' || buffer[i] == ']' );
        if (buffer[i] == ',') i++;

    } while ( (i < size) && buffer[i] != ']' );

    if (i < size) i++;
    //if (prevElementIdx != 0xFFFFFFFF) dom[prevElementIdx+2] = 0xFFFFFFFF;

    *indice = i;
    *domIdx = j;
    *domSz = dSz;

    return oIdx;

}


static uint32_t parseChildElement( uint8_t* buffer, uint32_t* indice, uint32_t size, uint32_t* dom, uint32_t* domIdx, uint32_t* domSz )
{
    uint32_t i = *indice;
    uint32_t j = *domIdx;
    uint32_t dSz = *domSz;
    uint32_t oIdx = 0xFFFFFFFF;

    skipSpaces( buffer, &i, size );

    switch (buffer[i])
    {
        case '"':
            oIdx = parseString( buffer, &i, size, dom, &j, &dSz );
            break;
        case '{':
            oIdx = parseObject( buffer, &i, size, dom, &j, &dSz );
            break;
        case '[':
            oIdx = parseArray( buffer, &i, size, dom, &j, &dSz );
            break;
        default:
            if ((buffer[i] >= '0' && buffer[i] <= '9') || (buffer[i] == '.') || (buffer[i] == '-'))
                oIdx = parseDouble( buffer, &i, size, dom, &j, &dSz );
            break;
    }

    *indice = i;
    *domIdx = j;
    *domSz = dSz;

    return oIdx;
}


extern "C" void turbojson_parsebuffer( struct JsonContext* ctx, uint8_t* jsonbuffer, uint32_t size, uint32_t allocsize )
{
    if (jsonbuffer != nullptr && size > 0 && allocsize > 0)
    {
        ctx->jsonbuffer = jsonbuffer;
        ctx->jsonbufferSize = size;
        ctx->jsonbufferMax = allocsize;

        if (ctx->dom == nullptr)
        {
            ctx->dom = (uint32_t*) align_alloc( MAX_CACHE_LINE_SIZE, allocsize*sizeof(uint32_t) );
            ctx->domSz = allocsize;
        }

        if (ctx->dom == nullptr) return;

        uint32_t i = 0;
        uint32_t j = 0;
        uint32_t dSz = ctx->domSz;

        skipSpaces( ctx->jsonbuffer, &i, ctx->jsonbufferSize );

        parseObject( ctx->jsonbuffer, &i, ctx->jsonbufferSize, ctx->dom, &j, &dSz );

        ctx->domIdx = j;
        ctx->domSz = dSz;
    }
}


extern "C" void turbojson_stringify( struct JsonContext* ctx )
{
    turbojson_pretty(ctx, false, 0, false);
}


static void prettyRec( uint8_t* jsonout, uint32_t* dom, uint8_t* jsonbuffer, uint32_t &i, uint32_t &j, uint32_t ident, bool spaces, uint32_t numberSpaces, bool linereturn )
{
    uint32_t sz, ci;

    for (uint32_t k=0; k<ident*numberSpaces; k++) jsonbuffer[j++] = spaces ? ' ' : '\t';

    switch (dom[i])
    {
    case TURBOJSON_DOM_STRING:
        sz = dom[i+2]-dom[i+1]+2;
        turbojson_memcpy(jsonout+j, jsonbuffer+dom[i+1]-1, jsonbuffer+dom[i+1]-1+sz);
        j += sz;
        i += 3;
        break;
    case TURBOJSON_DOM_REAL:
        sz = dom[i+2]-dom[i+1];
        turbojson_memcpy(jsonout+j, jsonbuffer+dom[i+1], jsonbuffer+dom[i+1]+sz);
        j += sz;
        i += 3;
        break;
    case TURBOJSON_DOM_OBJECT:
        jsonout[j++] = '{';
        if (linereturn) jsonbuffer[j++] = '\n';
        i += 2;
        prettyRec( jsonout, dom, jsonbuffer, i, j, ident+1, spaces, numberSpaces, linereturn );
        jsonout[j++] = '}';
        break;
    case TURBOJSON_DOM_MEMBER:
        sz = dom[i+2]-dom[i+1]+2;
        turbojson_memcpy(jsonout+j, jsonbuffer+dom[i+1]-1, jsonbuffer+dom[i+1]-1+sz);
        j += sz;
        if (numberSpaces) jsonout[j++] = ' ';
        jsonout[j++] = ':';
        if (numberSpaces) jsonout[j++] = ' ';
        ci = dom[i+3];
        prettyRec( jsonout, dom, jsonbuffer, ci, j, ident+1, spaces, numberSpaces, linereturn );
        if (dom[i+4] != 0xFFFFFFFF)
        {
            ci = dom[i+4];
            jsonout[j++] = ',';
            if (linereturn) jsonbuffer[j++] = '\n';
            prettyRec( jsonout, dom, jsonbuffer, ci, j, ident, spaces, numberSpaces, linereturn );
        }
        else i = ci;
        break;
    case TURBOJSON_DOM_ARRAY:
        jsonout[j++] = '[';
        if (linereturn) jsonbuffer[j++] = '\n';
        i += 2;
        prettyRec( jsonout, dom, jsonbuffer, i, j, ident+1, spaces, numberSpaces, linereturn );
        jsonout[j++] = ']';
        break;
    case TURBOJSON_DOM_ARRAY_ELEMENT:
        ci = dom[i+1];
        prettyRec( jsonout, dom, jsonbuffer, ci, j, ident+1, spaces, numberSpaces, linereturn );
        if (dom[i+2] != 0xFFFFFFFF)
        {
            ci = dom[i+2];
            jsonout[j++] = ',';
            if (linereturn) jsonbuffer[j++] = '\n';
            prettyRec( jsonout, dom, jsonbuffer, ci, j, ident, spaces, numberSpaces, linereturn );
        }
        else i = ci;
        break;
    default:
        break;
    }

    if (linereturn) jsonbuffer[j++] = '\n';
}


extern "C" void turbojson_pretty( struct JsonContext* ctx, bool spaces, uint32_t numberSpaces, bool linereturn )
{
    size_t domsz = numberSpaces*ctx->domSz;
    size_t reqoutsz = domsz > ctx->domSz*2 ? domsz : ctx->domSz*2;
    if (reqoutsz < 65536) reqoutsz = 65536;

    if (ctx->jsonout == nullptr || ctx->jsonoutMax<reqoutsz )
    {
        if (ctx->jsonout) align_free(ctx->jsonout);

        ctx->jsonout = (uint8_t*) align_alloc( MAX_CACHE_LINE_SIZE, reqoutsz );
        ctx->jsonoutMax = reqoutsz;
    }

    if (!ctx->jsonout) return;

    uint32_t i=0, j=0;

    prettyRec( ctx->jsonout, ctx->dom, ctx->jsonbuffer, i, j, 0, spaces, numberSpaces, linereturn );

    ctx->jsonoutIdx = j;
}


extern "C" void turbojson_writefile( struct JsonContext* ctx, const char* jsonfilename )
{
    if (ctx->jsonout == nullptr) return;

    FILE* out = fopen( jsonfilename, "wb" );

    if (out)
    {
        fwrite( ctx->jsonout, 1, ctx->jsonoutIdx, out );
        fclose(out);
    }
}

