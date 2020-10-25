#define EXPR_DEBUG_LOG 1
#include "expr.h"

int main()
{
    // clock_t start              = clock(), end;
    // char    expr[EXPR_MAX_LEN] = "y = ln abs(sin(x + sqrt(x-y))) + (x-2)^2";
    // // scanf("%[^n]", &expr);
    // // printf("expr")
    // printf("%lf\n", exprEval(expr, -1, 3, -1));
    // if (exprGetError()) printf("error flag: %d\n", exprGetError());
    // end = clock();
    // printf("Elapsed: %lf\n", (double)(end - start) / CLOCKS_PER_SEC);
    struct interval x = exprToInterval(0), y = exprToInterval(0);
    x.l = 0.625, x.r = 1.25;
    y.l = 0.664, y.r = 0.703;
    // struct interval r = exprEvalInterval("y = sinx", -1, x, y);
    struct interval r = exprAsin(exprCreateInterval(-1.387, -1.367));
    printf("%lf, %lf\n", r.l, r.r);
    printf("%d, %d\n", r.dl, r.dr);
}