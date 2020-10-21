#include "../ext.h"

const int width = 1280, height = 720;

const int dx = 5;

const int X = width / dx;
double    curveX[X + 10], curveY[X + 10];

int    n_sines = 3;
double A[100] = {150, 100, 125}, L[100] = {400, 500, 700}, P[100] = {0, 200, -100},
       C[100][3] = {{0, 0, 255}, {0, 255, 0}, {255, 0, 0}};

void drawSine(double A, double l, double p, double r, double g, double b)
{
    int    n = floor(-2 * p / l), i;
    double x, start, end;
    x = n * l / 2 + p;
    while (x < width) {
        start = x;
        end   = start + l / 2;
        for (i = 0; x <= end; i++, x += dx)
            curveX[i] = x, curveY[i] = height / 2 + A * sin(2 * PI / l * (x - p));
        x = curveX[i] = end;
        curveY[i]     = height / 2;
        iSetColorEx(r, g, b, 0.2);
        iFilledPolygon(curveX, curveY, i + 1);
        n--;
    }
    for (i = 0, x = -5; x <= width + 5; x += dx, i++)
        curveX[i] = x, curveY[i] = height / 2 + A * sin(2 * PI / l * (x - p));
    iSetColorEx(r, g, b, 1.0);
    iPath(curveX, curveY, i, 2);
}

void drawAxis()
{
    static double axisX[] = {0, width}, axisY[] = {height / 2, height / 2};
    iSetColorEx(255, 255, 255, 1);
    iPath(axisX, axisY, 2, 3);
}

void iDraw()
{
    iClear();
    for (int i = 0; i < n_sines; i++)
        drawSine(A[i], L[i], P[i], C[i][0], C[i][1], C[i][2]);
    drawAxis();
}

void iMouseMove(int mx, int my)
{
    // printf("x = %d, y= %d\n",mx,my);
    // place your codes here
}

void doubleClick(int button, int x, int y) {}

void click() {}

void iMouse(int button, int state, int x, int y)
{
    // static double last_click = iGetTime();
    // double        dt         = iGetTime() - last_click();
    y -= height / 2;
    if (state == GLUT_DOWN) {
        double minY       = height;
        int    sine_index = -1;
        for (int i = 0; i < n_sines; i++) {
            double Y = A[i] * sin(2 * PI / L[i] * (x - P[i]));
            if (Y < minY) {
                if ((0 <= y && y <= Y) || (0 >= y && y >= Y)) {
                    sine_index = i;
                    minY       = Y;
                }
            }
        }
        if (sine_index >= 0) {
            if (button == 3) { P[sine_index] += 10; }
            else if (button == 4) {
                P[sine_index] -= 10;
            }
        }
    }
}

void iKeyboard(unsigned char key)
{
    if (key == 'q') { exit(0); }
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
