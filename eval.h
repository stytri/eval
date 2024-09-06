#ifndef EVAL_H__INCLUDED
#define EVAL_H__INCLUDED  1
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

//------------------------------------------------------------------------------

#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

//------------------------------------------------------------------------------

struct evaluator {
	uintmax_t (*evaluator)(struct evaluator *, int, uintmax_t);
};

static uintmax_t eval_sequence(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval);

static uintmax_t nul_evaluator(struct evaluator *p, int o, uintmax_t u) {
	(void) p;
	(void) o;
	return u;
}

static inline int evalverify(char const *cs) {
	(void)eval_sequence(cs, &cs, NULL, NULL, 0);
	return !*cs;
}

static inline int eval(char const *cs, uintmax_t v[], struct evaluator *e, uintmax_t *p) {
	struct evaluator nul_e = { nul_evaluator };
	if(!e) {
		e = &nul_e;
	}
	uintmax_t u;
	if(!p) {
		p = &u;
	}
	*p = eval_sequence(cs, &cs, v, e, 1);
	return !*cs;
}

static inline char const *skipspace(char const *cs) {
	while(isspace(*cs)) {
		cs++;
	}
	return cs;
}

static uintmax_t eval_primary(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval) {
	uintmax_t l = 0;
	if(*(*csp = cs = skipspace(cs))) {
		int o = *cs;
		switch(o) {
		default :
			if(isdigit(*cs)) {
				char *s;
				l = strtoumax(cs, &s, 0);
				*csp = s;
				return l;
			}
			*csp = cs + 1;
			return l;
		case '%':
			o = *(cs + 1);
			if(isdigit(o)) {
				l = eval ? v[o - '0'] : 0;
				cs++;
			} else if(isalpha(o)) {
				l = eval ? v[10 + (toupper(o) - 'A')] : 0;
				cs++;
			}
			*csp = cs + 1;
			return l;
		case '(':
			l = eval_sequence(cs + 1, &cs, v, e, eval);
			if(*(*csp = cs = skipspace(cs)) == ')') {
				*csp = cs + 1;
			}
			return l;
		}
	}
	return l;
}

static uintmax_t eval_unary(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval) {
	uintmax_t l = 0;
	if(*(*csp = cs = skipspace(cs))) {
		int o = *cs;
		switch(o) {
		default :
			l = eval_primary(cs, csp, v, e, eval);
			return l;
		case '?':
			l = !!eval_unary(cs + 1, csp, v, e, eval);
			return l;
		case '!':
			l = !eval_unary(cs + 1, csp, v, e, eval);
			return l;
		case '~':
			l = ~eval_unary(cs + 1, csp, v, e, eval);
			return l;
		case '-':
			l = -eval_unary(cs + 1, csp, v, e, eval);
			return l;
		case '+':
			l = eval_unary(cs + 1, csp, v, e, eval);
			return l;
		case '<': case '>':
		case '&': case '|': case '^':
		case '*': case '/': case '\\':
		case '@': case '=':
			l = eval_unary(cs + 1, csp, v, e, eval);
			if(eval) {
				l = e->evaluator(e, o, l);
			}
			return l;
		}
	}
	return l;
}

static uintmax_t eval_exponention(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval) {
	uintmax_t l = eval_unary(cs, &cs, v, e, eval);
	while(*(*csp = cs = skipspace(cs))) {
		int o = *cs;
		switch(o) {
			uintmax_t r;
		case '<':
			switch(*(cs+1)) {
			case '<':
				r = eval_unary(cs + 2, &cs, v, e, eval);
				l = r < (sizeof(l) * CHAR_BIT) ? l << r : 0;
				continue;
			}
			return l;
		case '>':
			switch(*(cs+1)) {
			case '>':
				r = eval_unary(cs + 2, &cs, v, e, eval);
				l = r < (sizeof(l) * CHAR_BIT) ? l >> r : 0;
				continue;
			}
			return l;
		default :
			return l;
		}
	}
	return l;
}

static uintmax_t eval_multiplication(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval) {
	uintmax_t l = eval_exponention(cs, &cs, v, e, eval);
	while(*(*csp = cs = skipspace(cs))) {
		int o = *cs;
		switch(o) {
			uintmax_t r;
		case '*':
			r = eval_exponention(cs + 1, &cs, v, e, eval);
			l = l * r;
			continue;
		case '/':
			r = eval_exponention(cs + 1, &cs, v, e, eval);
			l = l / r;
			continue;
		case '\\':
			r = eval_exponention(cs + 1, &cs, v, e, eval);
			l = l % r;
			continue;
		default :
			return l;
		}
	}
	return l;
}

static uintmax_t eval_addition(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval) {
	uintmax_t l = eval_multiplication(cs, &cs, v, e, eval);
	while(*(*csp = cs = skipspace(cs))) {
		int o = *cs;
		switch(o) {
			uintmax_t r;
		case '+':
			r = eval_multiplication(cs + 1, &cs, v, e, eval);
			l = l + r;
			continue;
		case '-':
			r = eval_multiplication(cs + 1, &cs, v, e, eval);
			l = l - r;
			continue;
		default :
			return l;
		}
	}
	return l;
}

static uintmax_t eval_bitwise(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval) {
	uintmax_t l = eval_addition(cs, &cs, v, e, eval);
	while(*(*csp = cs = skipspace(cs))) {
		int o = *cs;
		switch(o) {
			uintmax_t r;
		case '&':
			switch(*(cs+1)) {
			case '&':
				return l;
			default :
				r = eval_addition(cs + 1, &cs, v, e, eval);
				l = l & r;
				continue;
			}
			return l;
		case '|':
			switch(*(cs+1)) {
			case '|':
				return l;
			default :
				r = eval_addition(cs + 1, &cs, v, e, eval);
				l = l | r;
				continue;
			}
			return l;
		case '^':
			r = eval_addition(cs + 1, &cs, v, e, eval);
			l = l ^ r;
			continue;
		default :
			return l;
		}
	}
	return l;
}

static uintmax_t eval_relation(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval) {
	uintmax_t l = eval_bitwise(cs, &cs, v, e, eval);
	while(*(*csp = cs = skipspace(cs))) {
		int o = *cs;
		switch(o) {
			uintmax_t r;
		case '=':
			switch(*(cs+1)) {
			case '=':
				r = eval_bitwise(cs + 2, &cs, v, e, eval);
				l = l && r;
				continue;
			}
			return l;
		case '<':
			switch(*(cs+1)) {
			case '>':
				r = eval_bitwise(cs + 2, &cs, v, e, eval);
				l = l != r;
				continue;
			case '=':
				r = eval_bitwise(cs + 2, &cs, v, e, eval);
				l = l <= r;
				continue;
			default :
				r = eval_bitwise(cs + 1, &cs, v, e, eval);
				l = l < r;
				continue;
			}
			return l;
		case '>':
			switch(*(cs+1)) {
			case '<':
				return l;
			case '=':
				r = eval_bitwise(cs + 2, &cs, v, e, eval);
				l = l >= r;
				continue;
			default :
				r = eval_bitwise(cs + 1, &cs, v, e, eval);
				l = l > r;
				continue;
			}
			return l;
		default :
			return l;
		}
	}
	return l;
}

static uintmax_t eval_boolean(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval) {
	uintmax_t l = eval_relation(cs, &cs, v, e, eval);
	while(*(*csp = cs = skipspace(cs))) {
		int o = *cs;
		switch(o) {
			uintmax_t r;
		case '&':
			switch(*(cs+1)) {
			case '&':
				r = eval_relation(cs + 2, &cs, v, e, eval);
				l = l && r;
				continue;
			}
			return l;
		case '|':
			switch(*(cs+1)) {
			case '|':
				r = eval_relation(cs + 2, &cs, v, e, eval);
				l = l || r;
				continue;
			}
			return l;
		case '>':
			switch(*(cs+1)) {
			case '<':
				r = eval_relation(cs + 2, &cs, v, e, eval);
				l = !!l ^ !!r;
				continue;
			}
			return l;
		default :
			return l;
		}
	}
	return l;
}

static uintmax_t eval_condition(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval);

static uintmax_t eval_alternation(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval, uintmax_t cond) {
	uintmax_t l = eval_condition(cs, &cs, v, e, eval && cond);
	if(*(*csp = cs = skipspace(cs)) == '!') {
		uintmax_t r = eval_condition(cs + 1, csp, v, e, eval && !cond);
		if(!cond) {
			return r;
		}
	}
	return l;
}

static uintmax_t eval_condition(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval) {
	uintmax_t l = eval_boolean(cs, &cs, v, e, eval);
	while(*(*csp = cs = skipspace(cs)) == '?') {
		l = eval_alternation(cs + 1, &cs, v, e, eval, l);
	}
	return l;
}

static uintmax_t eval_sequence(char const *cs, char const **csp, uintmax_t v[], struct evaluator *e, int eval) {
	uintmax_t l = eval_condition(cs, &cs, v, e, eval);
	while(*(*csp = cs = skipspace(cs)) == ',') {
		l = eval_condition(cs + 1, &cs, v, e, eval);
	}
	return l;
}

//------------------------------------------------------------------------------

#endif//ndef EVAL_H__INCLUDED
