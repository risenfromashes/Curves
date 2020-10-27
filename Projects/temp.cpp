#include "iGraphics.h"
#include <chrono>

#ifndef PI
#define PI 3.1415926535897932
#endif
#ifndef ERR
#define ERR 1e-8
#endif

#define min(x, y) ((x < y) ? (x) : (y))
#define max(x, y) ((x > y) ? (x) : (y))

struct point {
    double x, y;
};

static int transparent = 0;

void iSetColorEx(double r, double g, double b, double a)
{
    double mmx = 255;
    if (r > mmx) r = mmx;
    if (g > mmx) g = mmx;
    if (b > mmx) b = mmx;
    r /= mmx;
    g /= mmx;
    b /= mmx;
    glColor4f(r, g, b, a);
}

static point solve_sim_eqn(double a1, double b1, double c1, double a2, double b2, double c2)
{
    double       d = a1 * b2 - a2 * b1;
    struct point p;
    p.x = (b1 * c2 - b2 * c1) / d;
    p.y = (c1 * a2 - c2 * a1) / d;
    return p;
}
int isLeft(double x1, double y1, double x2, double y2, double x3, double y3)
{
    return ((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)) > 0 ? 1 : 0;
}
void iPath(double X[], double Y[], int n, double d = 1, int closed = 0)
{
    double pX[4], pY[4];
    double dy, dx, a1, b1, c1, a2, b2, c2, M1, M2;
    int    s1, s2;
    point  p1, p2;
    for (int i = 0; i < n + 2 * closed; i++) {
        if (i == n - 1 && !closed) {
            s2 = s2;
            a2 = a1, b2 = b1, c2 = c1;
        }
        else {
            dy = Y[(i + 1) % n] - Y[i % n];
            dx = X[(i + 1) % n] - X[i % n];
            if (i == 0)
                s2 = 1;
            else {
                s2 = (X[i % n] >= X[(i - 1) % n] && X[(i + 1) % n] >= X[i % n]) ||
                     (X[i % n] <= X[(i - 1) % n] && X[(i + 1) % n] <= X[i % n]);
                s2 = s1 * (2 * s2 - 1);
            }
            if (dx > 0)
                a2 = -dy, b2 = dx;
            else
                a2 = dy, b2 = -dx;
            c2 = -(a2 * X[i % n] + b2 * Y[i % n]);
            M2 = sqrt(a2 * a2 + b2 * b2);
        }
        if (i == 0 || i == n - 1 || fabs(a1 * b2 - a2 * b1) < ERR) {
            if (!closed) {
                a1 = b2, b1 = -a2, c1 = -(a1 * X[i % n] + b1 * Y[i % n]);
                p1 = solve_sim_eqn(a1, b1, c1, a2, b2, c2 + s2 * d * M2 / 2),
                p2 = solve_sim_eqn(a1, b1, c1, a2, b2, c2 - s2 * d * M2 / 2);
            }
        }
        else {
            p1 = solve_sim_eqn(a1, b1, c1 + s1 * d * M1 / 2, a2, b2, c2 + s2 * d * M2 / 2),
            p2 = solve_sim_eqn(a1, b1, c1 - s1 * d * M1 / 2, a2, b2, c2 - s2 * d * M2 / 2);
        }
        pX[2] = p1.x, pY[2] = p1.y;
        pX[3] = p2.x, pY[3] = p2.y;
        if (i != 0 && !(closed && (i == 1 || i == n - 1))) iFilledPolygon(pX, pY, 4);
        a1 = a2, b1 = b2, c1 = c2, M1 = M2, s1 = s2;
        pX[1] = pX[2], pY[1] = pY[2];
        pX[0] = pX[3], pY[0] = pY[3];
    }
}

void iRectangleEx(double x, double y, double w, double h, double d)
{
    double X[] = {x, x + w, x + w, x}, Y[] = {y, y, y + h, y + h};
    iPath(X, Y, 4, d, 0);
}

void iSetTransparency(int state) { transparent = (state == 0) ? 0 : 1; }

double iGetTime()
{
    static clock_t start = clock();
    return (clock() - start) / double(CLOCKS_PER_SEC);
}

double iRandom(double min, double max)
{
    static int f = 1;
    if (f) {
        srand((unsigned int)time(NULL));
        f = 0;
    }
    int s = 1 << 15;
    return min + (max - min) / s * (rand() % s);
}

void iRandomColor(double S, double V, double rgb[])
{
    // convert to rgb from random hue HSV
    int    d = 1 << 15;
    double H = iRandom(0, 360);
    double C = S * V;
    double X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
    double m = V - C;
    double r, g, b;
    if (H >= 0 && H < 60) { r = C, g = X, b = 0; }
    else if (H >= 60 && H < 120)
        r = X, g = C, b = 0;
    else if (H >= 120 && H < 180)
        r = 0, g = C, b = X;
    else if (H >= 180 && H < 240)
        r = 0, g = X, b = C;
    else if (H >= 240 && H < 300)
        r = X, g = 0, b = C;
    else
        r = C, g = 0, b = X;
    rgb[0] = (r + m) * 255;
    rgb[1] = (g + m) * 255;
    rgb[2] = (b + m) * 255;
}

void iResize(int width, int height);

void resizeFF(int width, int height)
{
    iScreenWidth  = width;
    iScreenHeight = height;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    iResize(width, height);
    glOrtho(0.0, iScreenWidth, 0.0, iScreenHeight, -1.0, 1.0);
    glViewport(0.0, 0.0, iScreenWidth, iScreenHeight);
    glutPostRedisplay();
}

void iInitializeEx(int width = 500, int height = 500, const char* title = "iGraphics")
{

    iScreenHeight = height;
    iScreenWidth  = width;
#ifdef FREEGLUT
    int   n = 1;
    char* p[1];
    glutInit(&n, p);
#endif
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_MULTISAMPLE);

    glutInitWindowSize(width, height);
    glutInitWindowPosition(10, 10);
    glutCreateWindow(title);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, width, 0.0, height, -1.0, 1.0);
    // glOrtho(-100.0 , 100.0 , -100.0 , 100.0 , -1.0 , 1.0) ;
    // SetTimer(0, 0, 10, timer_proc);

    iClear();

    glutDisplayFunc(displayFF);
    glutReshapeFunc(resizeFF);
    glutKeyboardFunc(keyboardHandler1FF); // normal
    glutSpecialFunc(keyboardHandler2FF);  // special keys
    glutMouseFunc(mouseHandlerFF);
    glutMotionFunc(mouseMoveHandlerFF);
    glutIdleFunc(animFF);

    //
    // Setup Alpha channel testing.
    // If alpha value is greater than 0, then those
    // pixels will be rendered. Otherwise, they would not be rendered
    //
    glAlphaFunc(GL_GREATER, 0.0f);
    glEnable(GL_ALPHA_TEST);

    if (transparent) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glutMainLoop();
}