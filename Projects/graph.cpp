#include "../ext.h"
#include <mmsystem.h>
const int width = 1280, height = 720;

const int steps = 20;
double    X[2048], Y[2048];

double eval(double x, double y)
{
    // return pow((x - 0.5), 2) + pow((y - 0.5), 2) - pow(300, 2);
    return x * x + y * y - 1.5 * 1.5;
}

void iDraw()
{
    static double (*F)(double, double) = &eval;
    iClear();
    int    i;
    double x, y, x0, y0, F0, du, dt, dx, dy, Fx, Fy, D;
    du = 1e-5;
    dt = 0.5;
    x = -1.5, y = 0;
    for (i = 0; i < steps; i++) {
        X[i] = height * x / 2 / 2 + width / 2, Y[i] = height * y / 2 / 2 + height / 2;
        F0 = F(x, y);
        Fx = (F(x + du, y) - F0) / du;
        Fy = (F(x, y + du) - F0) / du;
        D  = sqrt(Fx * Fx + Fy * Fy);
        dx = dt * Fy / D, dy = -dt * Fx / D;
        // if (dx < 0) dx = -dx, dy = -dy;
        x0 = x, y0 = y;
        x += dx, y += dy;
        for (int j = 0; j < 1; j++) {
            F0 = F(x, y);
            Fx = (F(x + du, y) - F0) / du;
            Fy = (F(x, y + du) - F0) / du;
            dx = x - x0;
            dy = y - y0;
            D  = Fx * dy - Fy * dx;
            x -= F0 * dy / D, y += F0 * dx / D;
        }
    }
    iPath(X, Y, steps, 2);
}

void iMouseMove(int mx, int my) {}

void iMouse(int button, int state, int mx, int my) {}

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
