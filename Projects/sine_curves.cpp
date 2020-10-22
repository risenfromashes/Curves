#include "../ext.h"

const int width = 1280, height = 720;

const int dx = 5;

const int N = width / dx + 1;

double X[N];

int    n_sines = 0;
double A[100], L[100], P[100], C[100][3];
double A0, L0;

int    clickedState = 0, selectedIndex = -1;
double X0, Y0;

void drawSines()
{
    static double pX[4], pY[4], qY[N], cY[N];
    int           crossesAxis;
    pY[2] = pY[3] = height / 2;
    for (int j = 0; j <= n_sines; j++) {
        for (int i = 0; i < N; i++) {
            if (j == 0) cY[i] = 0;
            pX[2] = pX[1] = X[i];
            if (j < n_sines) {
                qY[i] = pY[1] = height / 2 + A[j] * sin(2 * PI / L[j] * (X[i] - P[j]));
                cY[i] += qY[i] - height / 2;
                iSetColorEx(C[j][0], C[j][1], C[j][2], 0.2);
            }
            else {
                qY[i] = pY[1] = cY[i] += height / 2;
                iSetColorEx(255, 255, 255, 0.1);
            }
            if (i > 0) {
                crossesAxis =
                    (pY[0] >= height / 2 && pY[1] < height / 2) || (pY[0] < height / 2 && pY[1] >= height / 2);
                if (crossesAxis) {
                    // draw two triangles instead of a quadrilateral
                    pX[3] = pX[1], pY[3] = pY[1], pX[2] = pX[0];
                    pX[1] = (height / 2 - pY[0]) * (pX[1] - pX[0]) / (pY[1] - pY[0]) + pX[0], pY[1] = height / 2;
                    iFilledPolygon(pX, pY, 3);
                    pX[2] = pX[0] = pX[3], pY[0] = pY[3];
                    iFilledPolygon(pX, pY, 3);
                    pX[3] = pX[0], pY[3] = height / 2;
                    continue;
                }
                iFilledPolygon(pX, pY, 4);
            }
            pX[3] = pX[0] = pX[1], pY[0] = pY[1];
        }
        double stroke;
        if (j < n_sines) {
            iSetColorEx(C[j][0], C[j][1], C[j][2], 1.0);
            stroke = 2 + 2 * (j == selectedIndex);
        }
        else {
            iSetColorEx(255, 255, 255, 0.66);
            stroke = 3 + 2 * (j == selectedIndex);
        }
        iPath(X, qY, N, stroke);
    }
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
    drawSines();
    drawAxis();
}

int selectCurve(int x, int y)
{
    y -= height / 2;
    double minY       = height;
    int    sine_index = -1;
    for (int i = 0; i < n_sines; i++) {
        double Y = A[i] * sin(2 * PI / L[i] * (x - P[i]));
        if (fabs(Y) < fabs(minY)) {
            if ((0 <= y && y <= Y) || (0 >= y && y >= Y)) {
                sine_index = i;
                minY       = Y;
            }
        }
    }
    return sine_index;
}

void iMouseMove(int x, int y)
{
    if (clickedState) {
        int s            = (y >= height / 2) ? 1 : -1;
        L[selectedIndex] = L0 + x - X0;
        if ((Y0 >= height / 2 && y < height / 2) || (Y0 < height / 2 && y >= height / 2))
            A[selectedIndex] = -(A0 + s * (y - height + Y0));
        else
            A[selectedIndex] = A0 + s * (y - Y0);
    }
}

void iMouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) {
        if (button < 3) {
            int i = selectCurve(x, y);
            if (i >= 0) {
                selectedIndex = i;
                clickedState  = 1;
                X0 = x, Y0 = y;
                A0 = A[selectedIndex], L0 = L[selectedIndex];
            }
            else
                selectedIndex = -1;
        }
        else if (selectedIndex >= 0) {
            if (button == 3)
                P[selectedIndex] += 10;
            else if (button == 4)
                P[selectedIndex] -= 10;
        }
    }
    else {
        if (selectedIndex >= 0) {
            if (A[selectedIndex] < 0) P[selectedIndex] += L[selectedIndex] / 2, A[selectedIndex] = -A[selectedIndex];
        }
        clickedState = 0;
    }
}

void addSine()
{
    A[n_sines] = iRandom(50, height / 4);
    L[n_sines] = iRandom(200, width / 2);
    P[n_sines] = iRandom(0, width);
    iRandomColor(1, 1, C[n_sines]);
    n_sines++;
}
void removeSine() { n_sines--; }
void iKeyboard(unsigned char key)
{
    switch (key) {
        case 'q': exit(0); break;
        case '+':
            if (n_sines < 100) addSine();
            break;
        case '-':
            if (n_sines >= 0) removeSine();
            break;
        default: break;
    }
}

void iSpecialKeyboard(unsigned char key)
{

    if (key == GLUT_KEY_END) { exit(0); }
    // place your codes for other keys here
}

int main()
{
    double x;
    int    i;
    for (i = 0, x = 0; i < N; x += dx, i++)
        X[i] = x;
    for (i = 0; i < 3; i++)
        addSine();
    iSetTransparency(1);
    iInitializeEx(width, height, "Demo!");
    return 0;
}
