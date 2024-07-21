/*
TurboJson sample.

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
#include <string.h>
#include <time.h>


#include "../turbojson.h"
#include "../platform.h"


static void stringify( const char* in, const char* out )
{
    struct JsonContext* jsonctx = turbojson_allocateContext();
    clock_t start = clock();
    turbojson_parsefile( jsonctx, in );
    clock_t c1 = clock();
    printf("parse in %.6fs\n", double(c1-start)/CLOCKS_PER_SEC);
    //turbojson_stringify( jsonctx );
    clock_t c2 = clock();
    //printf("stringify in %.6fs\n", double(c2-c1)/CLOCKS_PER_SEC);
    turbojson_writefile( jsonctx, out );
    turbojson_freeContext( jsonctx );
}


static void pretty( const char* in, const char* out )
{
    struct JsonContext* jsonctx = turbojson_allocateContext();
    turbojson_parsefile( jsonctx, in );
    turbojson_pretty( jsonctx, true, 2 );
    turbojson_writefile( jsonctx, out );
    turbojson_freeContext( jsonctx );
}


int main( int argc, const char** argv )
{
    if (argc != 4)
    {
        printf("turbojson sample v0.2\n"
        "(C) 2024, Julien Perrier-cornet. Free software under BSD 3-Clause License.\n");
        return 1;
    }

    clock_t start = clock();

    if (strcmp(argv[1], "--stringify") == 0)
        stringify(argv[2], argv[3]);
    else if (strcmp(argv[1], "--pretty") == 0)
        pretty(argv[2], argv[3]);

    printf("%s -> %s in %.6fs\n", argv[2], argv[3],  double(clock()-start)/CLOCKS_PER_SEC);

    return 0;
}
