#include "../ext.h"

const int width = 1280, height = 720;

const int dx = 3;

const int X = width / dx;
double    curveX[X + 10], curveY[X + 10];

void iDraw()
{
    // double t, x;
    // t = iGetTime();
    // int i, n, last, side, convex, crossesAxis, left;
    // last = 0;
    // side = -2;
    // iClear();
    // double pX[4], pY[4];
    // pY[2] = pY[3] = height / 2;
    // for (i = 0, x = -dx; x <= width + dx; x += dx, i++) {
    //     pX[2] = pX[1] = x;
    //     pY[1]         = 50 + height / 2 + 200 * sin(2 * PI * (1 * t - x / 200)) * sin(2 * PI * (0.25 * t - x / 150));
    //     if (i > 0) {
    //         crossesAxis = (pY[0] >= height / 2 && pY[1] < height / 2) || (pY[0] < height / 2 && pY[1] >= height / 2);
    //         if (crossesAxis) {
    //             // draw two triangles instead of a quadrilateral
    //             pX[3] = pX[1], pY[3] = pY[1], pX[2] = pX[0];
    //             pX[1] = (height / 2 - pY[0]) * (pX[1] - pX[0]) / (pY[1] - pY[0]) + pX[0], pY[1] = height / 2;
    //             iFilledPolygon(pX, pY, 3);
    //             pX[2] = pX[0] = pX[3], pY[0] = pY[3];
    //             iFilledPolygon(pX, pY, 3);
    //             pX[3] = pX[0], pY[3] = height / 2;
    //             continue;
    //         }
    //         iFilledPolygon(pX, pY, 4);
    //     }
    //     pX[3] = pX[0] = pX[1], pY[0] = pY[1];
    // }
    iClear();
    // iSetColorEx(255, 0, 0, 1);
    // iRectangleEx(200, height / 2 - 100, 400, 200, 10);

    double X[] = {200, 800, 800, 200}, Y[] = {height / 2 - 100, height / 2 - 100, height / 2 + 100, height / 2 + 100};
    iPath(X, Y, 4, 10, 1, 1, 80, 20, 1);
}
void iPassiveMouseMove(int, int) {}
void iResize(int x, int y) {}
void iMouseMove(int mx, int my)
{
    // printf("x = %d, y= %d\n",mx,my);
    // place your codes here
}

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
    for (int i = 0; i < width; i++)
        curveX[i] = i * dx;
    iSetTransparency(1);
    iInitializeEx(width, height, "Demo!");
    return 0;
}
