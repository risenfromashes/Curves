#include "../ext.h"
#include "expr.h"

const int width = 1280, height = 720;

#define MAX_POINTS 4096

const int steps = 20;
double    X[MAX_POINTS], Y[MAX_POINTS];
double    graphH = 5, graphW = 5;

#define GRID_SIZE 512

char expr[256] = "";
char expr_pos  = 0;
int  changed   = 0;

void reverseArray(double arr[], int n)
{
    for (int i = 0; i < n / 2; i++) {
        double t       = arr[i];
        arr[i]         = arr[n - i - 1];
        arr[n - i - 1] = t;
    }
}

// set all neighbours to zero
void markDone(double G[GRID_SIZE][GRID_SIZE], int h, int v)
{
    G[h][v] = 0;
    if (h > 0) {
        G[h - 1][v] = 0;
        if (v > 0) G[h - 1][v - 1] = G[h][v - 1] = 0;
        if (v < GRID_SIZE - 1) G[h - 1][v + 1] = G[h][v + 1] = 0;
    }
    if (h < GRID_SIZE - 1) {
        G[h + 1][v] = 0;
        if (v > 0) G[h + 1][v - 1] = 0;
        if (v < GRID_SIZE - 1) G[h + 1][v + 1] = 0;
    }
}

// ref: http://web.mit.edu/18.06/www/Spring17/Multidimensional-Newton.pdf
void trace(double G[GRID_SIZE][GRID_SIZE])
{
#define F(x, y) exprEval(expr, -2, x, y)
    const double err = 1e-7;
    double       x, y, x0, y0, F0, du, dt, dx, dy, Fx, Fy, D, d;
    du = 1e-5;
    dt = 10.0 / GRID_SIZE;
    x = -1.5, y = 0;
    for (int h = 0; h < GRID_SIZE; h++) {
        for (int v = 0; v < GRID_SIZE; v++) {
            if (G[h][v]) {
                // try all the sides of the bounding box for the closest solution
                struct point tl, tr, bl, br;
                double       l, r, t, b, Fmin;
                struct point p[4];
                int          i, nearest;
                double       F0, Fx, Fy;
                for (i = 0; i <= 3; i++) {
                    for (int j = 0; j < 4; j++) {
                        if (i == 0) {
                            switch (j) {
                                case 0: // left-side
                                    p[j].x = l, p[j].y = (t + b) / 2;
                                    break;
                                case 1: // right-side
                                    p[j].x = r, p[j].y = (t + b) / 2;
                                    break;
                                case 2: // top-side
                                    p[j].x = (l + r) / 2, p[j].y = t;
                                    break;
                                case 3: // bottom-side
                                    p[j].x = (l + r) / 2, p[j].y = b;
                                    break;
                            }
                        }
                        else {
                            if (j < 2) {
                                Fy = F(p[j].x, p[j].y + du) / du;
                                p[j].y += F0 / Fy;
                            }
                            else {
                                Fx = F(p[j].x + du, p[j].y) / du;
                                p[j].x += F0 / Fx;
                            }
                        }
                        F0 = F(p[j].x, p[j].y);
                        if (i == 3 && fabs(F0) < fabs(Fmin)) { // only take min on the last run
                            nearest = j;
                            Fmin    = F0;
                        }
                    }
                }
                if (fabs(Fmin) > err) {
                    markDone(G, h, v);
                    continue;
                }
                double initX, initY, initDirX, initDirY;
                int    rev = 0;
                F0         = Fmin;
                initX = x = p[nearest].x, initY = y = p[nearest].y;
                // iterate in the inital direction and after following the trail a while
                // go back to the initial point and go the other way
                for (i = 0; i < MAX_POINTS; i++) {
                    // X[i] = height * x / 2 / 2 + width / 2, Y[i] = height * y / 2 / 2 + height / 2;
                    for (int j = 0; j <= 3; j++) {
                        Fx = (F(x + du, y) - F0) / du;
                        Fy = (F(x, y + du) - F0) / du;
                        if (j == 0) {
                            x0 = x, y0 = y;
                            D  = sqrt(Fx * Fx + Fy * Fy);
                            dx = dt * Fy / D, dy = -dt * Fx / D;
                            if (i == 0) initDirX = dx, initDirY = dy;
                            x += dx, y += dy;
                        }
                        else {
                            dx = x - x0;
                            dy = y - y0;
                            D  = Fx * dy - Fy * dx;
                            x -= F0 * dy / D, y += F0 * dx / D;
                        }
                        // if (dx < 0) dx = -dx, dy = -dy;
                        x0 = x, y0 = y;
                        x += dx, y += dy;
                        F0 = F(x, y);
                        Fx = (F(x + du, y) - F0) / du;
                        Fy = (F(x, y + du) - F0) / du;
                        dx = x - x0;
                        dy = y - y0;
                        F0 = F(x, y);
                    }
                    if (fabs(F0) > err || x < -5.0 || x > 5.0 || y < -5.0 || y > 5.0 || i >= MAX_POINTS / 2) {
                        if (rev)
                            break;
                        else {
                            reverseArray(X, i + 1);
                            reverseArray(Y, i + 1);
                            rev = 1;
                            x = initX - initDirX, y = initY - initDirY;
                        }
                    }
                }
            }
        }
    }
#undef F
    iPath(X, Y, steps, 2);
}
void drawTextBox()
{
    iRectangle(0, 0, width, 36);
    iText(10, 10, expr, GLUT_BITMAP_HELVETICA_18);
}

void solveExpr()
{
    if (strlen(expr) > 0) {
        int G[GRID_SIZE][GRID_SIZE] = {1};
        int T                       = round(log2(GRID_SIZE));
        for (int k = 0; k <= T; k++) {
            int P = 1 << k;
            int Q = GRID_SIZE / P;
            for (int h = 0; h < GRID_SIZE; h += Q) {
                for (int v = 0; v < GRID_SIZE; v += Q) {
                    // starts from top-left
                    if (G[h][v]) {
                        // if (changed && k < 4) printf("h: %d, v: %d, P: %d\n", h, v, P);
                        struct interval x = exprCreateInterval(-5.0 + 10.0 * h / GRID_SIZE,
                                                               -5.0 + 10.0 * (h + Q) / GRID_SIZE),
                                        y = exprCreateInterval(5.0 - 10.0 * (v + Q) / GRID_SIZE,
                                                               5.0 - 10.0 * v / GRID_SIZE);
                        struct interval r = exprEvalInterval(expr, -2 + changed, x, y);
                        if (changed) changed = 0;
                        // if (changed && k < 4)
                        //     printf("([%lf,%lf],[%lf,%lf])->[%lf,%lf] <%d,%d>\n",
                        //            x.l,
                        //            x.r,
                        //            y.l,
                        //            y.r,
                        //            r.l,
                        //            r.r,
                        //            r.def,
                        //            r.cont);
                        if (r.def && (r.l <= 0 && 0 <= r.r)) {
                            if (P == GRID_SIZE) {
                                if (r.cont && isfinite(r.l) && isfinite(r.r)) {
                                    G[h][v]  = 1;
                                    double d = (double)width / GRID_SIZE;
                                    double x = h * d, y = -v * d + (width + height) / 2.0;
                                    iRectangle(x, y - d, d, d);
                                }
                            }
                            else {
                                int d       = GRID_SIZE / P / 2;
                                G[h + d][v] = G[h + d][v + d] = G[h][v + d] = 1;
                            }
                        }
                        else
                            G[h][k] = 0;
                    }
                }
            }
        }
    }
}
void iDraw()
{
    iClear();
    drawTextBox();
    solveExpr();
}
void iResize(int, int) {}
void iMouseMove(int mx, int my) {}

void iMouse(int button, int state, int mx, int my)
{
    if (state == GLUT_DOWN) {
        double d = (double)width / GRID_SIZE;
        int    h = round(mx / d), v = round((-my + (width + height) / 2) / d);
        printf("h: %d, v: %d, P: %d\n", h, v);
        struct interval x = exprCreateInterval(-5.0 + 10.0 * h / GRID_SIZE, -5.0 + 10.0 * (h + 1) / GRID_SIZE),
                        y = exprCreateInterval(5.0 - 10.0 * (v + 1) / GRID_SIZE, 5.0 - 10.0 * v / GRID_SIZE);
        struct interval r = exprEvalInterval(expr, -1, x, y);
        printf("([%lf,%lf],[%lf,%lf])->[%lf,%lf] <%d,%d>\n", x.l, x.r, y.l, y.r, r.l, r.r, r.def, r.cont);
    }
}

void iKeyboard(unsigned char key)
{
    changed = 1;
    if (isalnum(key))
        expr[expr_pos++] = (char)key;
    else {
        switch (key) {
            case ' ':
            case '=':
            case '*':
            case '/':
            case '+':
            case '-':
            case '^':
            case '.':
            case '(':
            case ')': expr[expr_pos++] = (char)key; break;
            case '\b':
                if (expr_pos > 0) expr[--expr_pos] = '\0';
                break;
            default: break;
        }
    }
    // place your codes for other keys here
}

void iSpecialKeyboard(unsigned char key)
{

    if (key == GLUT_KEY_END) { exit(0); }
    // place your codes for other keys here
}

int main()
{

    iSetTransparency(1);
    iInitializeEx(width, height, "Demo!");
    return 0;
}
