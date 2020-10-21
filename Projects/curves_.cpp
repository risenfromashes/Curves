#include "../ext.h"

const int width = 1280, height = 720;

const int dx = 3;

const int X = width/dx;
double curveX[X + 2], curveY[X + 2];


void iDraw()
{
    double t = iGetTime();
    int last = 0;
    iClear();
    for (int i = 0; i < X; i++)
    {
        curveY[i] = height / 2 + 200 * sin(2 * PI * (3 * t - curveX[i] / 200));
        if (i != 0)
        {
            //only draw filled polygons if the curve crosses x-axis
            if ((curveY[i - 1] - height / 2) / (curveY[i] - height / 2) < 0 || i == (X - 1))
            {
                //insert 3 points to close curve with x-axis
                //changing next 2 points
                iSetColorEx(9, 132, 227, 1.0);
                double tempX = curveX[i], tempY = curveY[i];
                curveX[i] = curveX[i];
                curveY[i] = curveY[i + 1] = height / 2;
                curveX[i + 1] = curveX[i + 2] = curveX[last];
                curveY[i + 2] = curveY[last];
                iFilledPolygon(&curveX[last], &curveY[last], i - last + 3);
                //reverting back points
                curveX[i] = tempX, curveY[i] = tempY;
                curveX[i + 1] = dx * (i + 1);
                curveX[i + 2] = dx * (i + 2);
                last = i;
                // iSetColorEx(255, 0, 0, 1.0);
                // iFilledCircle(curveX[last], curveY[last], 5, 100);
            }
        }
    }
    
}

void iMouseMove(int mx, int my)
{
    //printf("x = %d, y= %d\n",mx,my);
    //place your codes here
}

void iMouse(int button, int state, int mx, int my)
{
}

void iKeyboard(unsigned char key)
{
    if (key == 'q')
    {
        exit(0);
    }
    //place your codes for other keys here
}

void iSpecialKeyboard(unsigned char key)
{

    if (key == GLUT_KEY_END)
    {
        exit(0);
    }
    //place your codes for other keys here
}

int main()
{
    for(int i = 0; i < width; i++)
        curveX[i] =  i * dx;
    iSetTransparency(1);
    iInitializeEx(width, height, "Demo!");
    return 0;
}
