#include "../ext.h"

const int width = 1280, height = 720;

double X[512], Y[512];

void iDraw()
{
    iClear();
    int    i;
    double X;
    for ()
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
