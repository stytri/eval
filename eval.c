/*
MIT License

Copyright (c) 2024 Tristan Styles

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and atsociated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Build with https://github.com/stytri/m
//
// ::compile
// :+  $CC $CFLAGS $XFLAGS $SMALL-BINARY
// :+      -o $+^ $"* $"!
//
// ::clean
// :+  $RM *.i *.s *.o *.exe
//
// ::-
// :+  $CC $CFLAGS
// :+      -Og -g -DDEBUG_$: -o $+: $"!
// :&  $DBG -tui --args $+: $"*
// :&  $RM $+:
//
// ::CFLAGS!CFLAGS
// :+      $CSTD -Wall -Wextra $WINFLAGS $INCLUDE
//
// ::XFLAGS!XFLAGS
// :+      -DNDEBUG=1 -O3 -march=native
//
// ::SMALL-BINARY
// :+      -fmerge-all-constants -ffunction-sections -fdata-sections
// :+      -fno-unwind-tables -fno-asynchronous-unwind-tables
// :+      -Wl,--gc-sections -s
//
// ::CSTD!CSTD
// :+      -std=c23
//
// ::windir?WINFLAGS
// :+      -D__USE_MINGW_ANSI_STDIO=1
//

//------------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "eval.h"

//------------------------------------------------------------------------------

static void usage(FILE *out) {
	fputs("Evaluate expression\n\n", out);
	fputs("usage: eval [OPTIONS] EXPRESSION VALUE...\n", out);
	fputs("OPTIONS:\n", out);
	fputs("\t-h        display this help\n", out);
	fputs("\t-x        output hexadecimal\n", out);
}

int main(int argc, char *argv[]) {
	int hex = 0;
	int argi = 1;
	while((argi < argc) && (argv[argi][0] == '-')) {
		char *args = argv[argi] + 1;
		if(isdigit(*args)) {
			break;
		}
		argi++;
		while(*args) switch(*args++) {
		case 'h':
			usage(stdout);
			return EXIT_SUCCESS;
		case 'x':
			hex = 1;
			break;
		}
	}
	if(argi >= argc) {
		usage(stderr);
		return EXIT_SUCCESS;
	}
	char const *expr = argv[argi++];
	uintmax_t   v[36];
	int i = 0;
	for(; (argi < argc) && (i < 10); argi++, i++) {
		v[i] = strtoumax(argv[argi], NULL, 0);
	}
	for(; i < 36; i++) {
		v[i] = 0;
	}
	v[10 + ('B' - 'A')] = CHAR_BIT;
	v[10 + ('W' - 'A')] = sizeof(v[0]);
	struct timespec t;
	timespec_get(&t, TIME_UTC);
	v[10 + ('T' - 'A')] = (uintmax_t)t.tv_sec * UINTMAX_C(1000000000) + t.tv_nsec;
	uintmax_t u;
	if(eval(expr, v, 0, &u)) {
		printf(hex ? "%#jx\n" : "%ju\n", u);
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}
