#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#define PI  3.1415926535897932
#define ERR 1e-8

#define EXPR_MAX_LEN 128

#define EXPR_DEBUG_LOG 1

static unsigned int error_flags = 0;
// error enums
#define EXPR_UNMATCHED_BRACKET 1
#define EXPR_UNKNOWN_SYMBOL    2
#define EXPR_UNDEFINED         4
#define EXPR_DISCONTINUOUS     8
// function enums
#define EXPR_SIN  0
#define EXPR_COS  1
#define EXPR_TAN  2
#define EXPR_ASIN 3
#define EXPR_ACOS 4
#define EXPR_ATAN 5
#define EXPR_LOG  6
#define EXPR_ABS  7
#define EXPR_SQRT 8
#define EXPR_CBRT 9

int exprGetError() { return error_flags; }

// l = -1 indicates the string is new and its the first call
// l = -2 indicates its the same as the one in the last call
// l = -3 indicates until_op
// indicates the index to end otherwise
double exprEval(const char* expr, int l, double x, double y)
{
#if EXPR_DEBUG_LOG
    printf("Called with l: %d\n", l);
#endif
    static int bracketEnds[EXPR_MAX_LEN + 1];
    static int i, len;
    int        until_op = 0;
    if (l == -1 || l == -2) { // new evaluation
        i = 0;                // reset counter
        error_flags &= 3;     // clear point specific flags
        if (l == -1) {        // if string is new
            error_flags = 0;  // clear all flags
            len         = strlen(expr);
            // remember bracket endings
            int i, r;
            for (i = len - 1, r = -1; i >= 0; i--) {
                if (expr[i] == ')')
                    bracketEnds[i] = r, r = i;
                else if (expr[i] == '(') {
                    if (r == -1) {
                        error_flags |= EXPR_UNMATCHED_BRACKET;
                        return 0;
                    }
                    bracketEnds[i] = r, r = bracketEnds[r];
                }
            }
            if (r != -1) {
                error_flags |= EXPR_UNMATCHED_BRACKET;
                return 0;
            }
#if EXPR_DEBUG_LOG
            for (int i = 0; i < len; i++)
                printf("%2d ", i);
            printf("\n");
            for (int i = 0; i < len; i++)
                printf("%2d ", bracketEnds[i]);
            printf("\n");
            for (int i = 0; i < len; i++)
                printf(" %c ", expr[i]);
            printf("\n");
#endif
        }
        l = len;
    }
    else if (l == -3) {
        until_op = 1;
        l        = len;
    }
    int    denom = 0;
    double s, p[2], c, t;
    c = s = 0.0;
    p[0] = p[1] = 1;

#if EXPR_DEBUG_LOG
    printf("Starting loop with l: %d, len: %d\n", l, len);
#endif
    for (; i <= l; i++) {
        while (expr[i] == ' ')
            i++;
#if EXPR_DEBUG_LOG
        if (until_op) printf("[until op] ");
        if (denom) printf("%d/%d) %0.1f + %0.1f/(%0.1f) [%0.1f] -%c->\n", i, l, s, p[0], p[1], c, expr[i]);
        printf("%d/%d) %0.1f + (%0.1f)/%0.1f [%0.1f] -%c->\n", i, l, s, p[0], p[1], c, expr[i]);
#endif
        if (expr[i] == 'x')
            p[denom] *= c = x;
        else if (expr[i] == 'y')
            p[denom] *= c = y;
        else if (expr[i] == 'e')
            p[denom] *= c = exp(1);
        else if (!strnicmp(expr + i, "pi", 2))
            p[denom] *= c = PI, i++;
        else if (isdigit(expr[i]) || expr[i] == '.') {
            char* end;
            p[denom] *= c = strtod(expr + i, &end);
            i             = end - expr - 1;
        }
        else if (expr[i] == '^') {
            i++, t = exprEval(expr, -3, x, y);
            if (exprGetError()) return 0;
            p[denom] *= c = pow(c, t - 1);
        }
        else if (expr[i] == '+' || expr[i] == '-' || expr[i] == '=' || expr[i] == ')' || expr[i] == '\0') {
            if (until_op) break;
            if (c != 0.0) s += p[0] / p[1];
            p[0] = p[1] = 1, denom = 0;
            if (expr[i] == '-')
                p[0] = -1;
            else if (expr[i] == '=')
                s = -s;
            else if (expr[i] == ')') {
#if EXPR_DEBUG_LOG
                printf("[bracket call] Returning %0.1f (%d/%d)\n", s, i, l);
#endif
                return s;
            }
            else if (expr[i] == '\0')
                return s;
        }
        else if (expr[i] == '*') {
            if (until_op) break;
            denom = 0;
            t     = exprEval(expr, -3, x, y);
            if (exprGetError()) return 0;
            p[denom] *= c = t;
        }
        else if (expr[i] == '/') {
            if (until_op) break;
            denom = 1;
        }
        else if (expr[i] == '(') {
            t = exprEval(expr, bracketEnds[i++], x, y);
            if (exprGetError()) return 0;
            p[denom] *= c = t;
        }
        else {
            int f;
            if (!strncmp(expr + i, "sin", 3))
                i += 3, f = EXPR_SIN;
            else if (!strncmp(expr + i, "cos", 3))
                i += 3, f = EXPR_COS;
            else if (!strncmp(expr + i, "tan", 3))
                i += 3, f = EXPR_TAN;
            else if (!strncmp(expr + i, "arcsin", 6))
                i += 6, f = EXPR_ASIN;
            else if (!strncmp(expr + i, "arccos", 6))
                i += 6, f = EXPR_ACOS;
            else if (!strncmp(expr + i, "arctan", 6))
                i += 6, f = EXPR_ATAN;
            else if (!strncmp(expr + i, "ln", 2))
                i += 2, f = EXPR_LOG;
            else if (!strncmp(expr + i, "log", 3))
                i += 3, f = EXPR_LOG;
            else if (!strncmp(expr + i, "sqrt", 4))
                i += 4, f = EXPR_SQRT;
            else if (!strncmp(expr + i, "cbrt", 4))
                i += 4, f = EXPR_CBRT;
            else if (!strncmp(expr + i, "abs", 3))
                i += 3, f = EXPR_ABS;
            else {
                error_flags |= EXPR_UNKNOWN_SYMBOL;
                printf("ERROR: Unkown symbol at %d [%d] (l: %d)\n", i, (int)expr[i], l);
                return 0;
            }
            if (expr[i] == '(')
                t = exprEval(expr, bracketEnds[i++], x, y);
            else
                t = exprEval(expr, -3, x, y);
            if (exprGetError()) return 0;
            switch (f) {
                case EXPR_SIN: p[denom] *= c = sin(t); break;
                case EXPR_COS: p[denom] *= c = cos(t); break;
                case EXPR_TAN:
                    if (fabs(round(t * 2 / PI) * PI / 2 - t) < 1e-5) {
                        error_flags |= EXPR_DISCONTINUOUS;
                        return 0;
                    }
                    p[denom] *= c = tan(t);
                    break;
                case EXPR_ASIN:
                case EXPR_ACOS:
                    if (t > 1.0 || t < -1.0) {
                        error_flags |= EXPR_UNDEFINED;
                        return 0;
                    }
                    if (f == EXPR_ASIN)
                        p[denom] *= c = asin(t);
                    else
                        p[denom] *= c = acos(t);
                    break;
                case EXPR_ATAN: p[denom] *= c = atan(t); break;
                case EXPR_LOG:
                    if (t <= 0) {
                        error_flags |= EXPR_DISCONTINUOUS;
                        return 0;
                    }
                    p[denom] *= c = log(t);
                    break;
                case EXPR_ABS: p[denom] *= c = fabs(t); break;
                case EXPR_SQRT:
                    if (t < 0) {
                        error_flags |= EXPR_UNDEFINED;
                        return 0;
                    }
                    p[denom] *= c = sqrt(t);
                    break;
                case EXPR_CBRT: p[denom] *= c = cbrt(t); break;
            }
        }
#if EXPR_DEBUG_LOG
        if (denom) printf("%d/%d) %0.1f + %0.1f/(%0.1f) [%0.1f]\n", i, l, s, p[0], p[1], c);
        printf("%d/%d) %0.1f + (%0.1f)/%0.1f [%0.1f]\n", i, l, s, p[0], p[1], c);
#endif
    }
    if (until_op) {
#if EXPR_DEBUG_LOG
        printf("[until op] Returning %0.1f\n", p[0]);
#endif
        i--;
        return p[0];
    }
    return s;
}

int main()
{
    clock_t start              = clock(), end;
    char    expr[EXPR_MAX_LEN] = "y = ln abs(sin(x + sqrt(x-y))) + (x-2)^2";
    // scanf("%[^n]", &expr);
    // printf("expr")
    printf("%lf\n", exprEval(expr, -1, 3, -1));
    if (exprGetError()) printf("error flag: %d\n", exprGetError());
    end = clock();
    printf("Elapsed: %0.1f\n", (double)(end - start) / CLOCKS_PER_SEC);
}