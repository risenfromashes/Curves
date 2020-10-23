#include "../ext.h"

const int width = 1280, height = 720;

const int dx = 3;

const int N = width / dx + 2;

double X[N];

int    n_sines = 0;
double A[100], L[100], P[100], C[100][3];
double A0[100], L0[100], P0[100];
int    selected[100] = {0}, n_selected = 0;
int    clickedState = 0;
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
            stroke = 2;
            if (selected[j]) stroke += 2;
        }
        else {
            iSetColorEx(255, 255, 255, 0.75);
            stroke = 3 + 2 * (n_selected == n_sines);
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
    double Y, C, minY = height;
    int    sine_index = -1;
    for (int i = 0; i <= n_sines; i++) {
        if (i < n_sines) {
            Y = A[i] * sin(2 * PI / L[i] * (x - P[i]));
            C += Y;
        }
        else
            Y = C;
        if (fabs(Y) < fabs(minY)) {
            if ((0 <= y && y <= Y) || (0 >= y && y >= Y)) {
                sine_index = i;
                minY       = Y;
            }
        }
    }
    if (sine_index == -1) return 0;
    int all = sine_index == n_sines, ctrlPressed = glutGetModifiers() & GLUT_ACTIVE_CTRL;
    if (selected[sine_index]) { // if already selected, mark others to be deselected on mouse up
        for (int i = 0; i < n_sines; i++)
            if (selected[i] && i != sine_index) selected[i] = -1;
        return 2;
    }
    else if (all) { // select all
        for (int i = 0; i < n_sines; i++)
            selected[i] = 1;
        n_selected = n_sines;
    }
    else if (ctrlPressed) { // just add this to selected if ctrl is pressed
        if (!selected[sine_index]) n_selected++;
        selected[sine_index] = 1;
    }
    else { // just select this
        for (int i = 0; i < n_sines; i++)
            selected[i] = i == sine_index;
        n_selected = 1;
    }
    return 1;
}

void iMouseMove(int x, int y)
{
    if (clickedState && n_selected > 0) {
        int s, i, invert;
        s      = (y >= height / 2) ? 1 : -1;
        invert = (Y0 >= height / 2 && y < height / 2) || (Y0 < height / 2 && y >= height / 2);
        for (i = 0; i < n_sines; i++) {
            if (selected[i]) {
                double X = round(4 * (X0 - P0[i]) / L0[i]) * L0[i] / 4 + P0[i];
                L[i]     = L0[i] + x - X0;
                P[i]     = X - L[i] / L0[i] * (X - P0[i]);
                if (invert)
                    A[i] = -(A0[i] + s * (y - height + Y0));
                else
                    A[i] = A0[i] + s * (y - Y0);
            }
        }
    }
}

void iMouse(int button, int state, int x, int y)
{
    static int shouldDeselect = 0;
    if (state == GLUT_DOWN) {
        clickedState = 1;
        if (button < 3) {
            int r          = selectCurve(x, y);
            shouldDeselect = r > 1;
            if (r) {
                X0 = x, Y0 = y;
                for (int j = 0; j < n_sines; j++)
                    if (selected[j]) A0[j] = A[j], L0[j] = L[j], P0[j] = P[j];
            }
            else {
                for (int j = 0; j < n_sines; j++)
                    selected[j] = 0;
                n_selected = 0;
            }
        }
        else if (n_selected > 0) {
            if (button == 3 || button == 4) {
                int s = button == 3 ? 1 : -1;
                for (int j = 0; j < n_sines; j++)
                    if (selected[j]) P[j] = P[j] + s * 10;
            }
        }
    }
    else {
        if (n_selected > 0) {
            if (shouldDeselect) {
                if (x == X0 && y == Y0) {
                    for (int j = 0; j < n_sines; j++)
                        if (selected[j] < 0) selected[j] = 0;
                    n_selected = 0;
                }
                else {
                    for (int j = 0; j < n_sines; j++)
                        if (selected[j] < 0) selected[j] = 1;
                }
                shouldDeselect = 0;
            }
            // replace negative amplitude with L/2 phase shift
            for (int j = 0; j < n_sines; j++)
                if (selected[j] && A[j] < 0) P[j] += L[j] / 2, A[j] = -A[j];
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

void removeSine()
{
    if (n_selected > 0) {
        int i, r;
        for (i = 0, r = 0; i < n_sines; i++) {
            if (selected[i]) {
                r++;
                selected[i] = 0;
            }
            else if (r > 0) {
                A[i - r] = A[i], L[i - r] = L[i], P[i - r] = P[i];
                C[i - r][0] = C[i][0], C[i - r][1] = C[i][1], C[i - r][2] = C[i][2];
            }
        }
        n_sines -= r;
    }
    else
        n_sines--;
    n_selected = 0;
}
void iKeyboard(unsigned char key)
{
    switch (key) {
        case 'q': exit(0); break;
        case '+':
            if (n_sines < 100) addSine();
            break;
        case '-':
        case (unsigned char)127:
            if (n_sines >= 0) removeSine();
            break;
        case 1: {
            for (int i = 0; i < n_sines; i++)
                selected[i] = 1;
            n_selected = n_sines;
        } break;
        default: break;
    }
}

void iSpecialKeyboard(unsigned char key)
{

    if (key == GLUT_KEY_END) { exit(0); }
    switch (key) {
        case GLUT_KEY_END: exit(0); break;
    }
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
