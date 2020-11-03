#define EXPR_DEBUG_LOG 1
#include "expr.h"
// int reverseBits(int v, int p)
// // {
// //     int ret = 0;
// //     for (int i = 0; i < p; i++)
// //         if (v & (1 << i)) ret |= 1 << (p - i - 1);
// //     printf("%d, %d\n", v, ret);
// //     return ret;
// // }

// // void fft(double X[])
// // {
// //     int n = 8;
// //     for (int i = 0; i < n / 2; i++) {
// //         double t = X[i];
// //         int    r = reverseBits(i, 3);
// //         X[i]     = X[r];
// //         X[r]     = t;
// //     }
// //     complex double
// // }
int main()
{
    // {
    char expr[EXPR_MAX_LEN] = " y=sinhx";
    // scanf("%[^n]", &expr);
    // printf("expr")
    printf("%lf\n", exprEval(expr, -1, 1, 1));
    if (exprGetError()) printf("error flag: %d\n", exprGetError());
    // struct interval x, y;
    // x = exprCreateInterval(1, 2);
    // y = exprCreateInterval(1, 2);
    // // struct interval r = exprEvalInterval("pi", -1, x, y);
    // struct interval r = exprSinh(x);
    // printf("%lf, %lf\n", r.l, r.r);
    // printf("%d, %d\n", r.def, r.cont);
    // // printf("%lf", fmod(-3.5, 2));
    printf("%lf", tan(-PI / 2));
}