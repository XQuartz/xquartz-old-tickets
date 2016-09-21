#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glut.h>
#include <GL/gl.h>

int window2 = 0, window = 0, width = 400, height = 400;

void display(void)
{
  glutSetWindow(window);
glClearColor(0.0, 1.0, 1.0, 1.0);
glClear(GL_COLOR_BUFFER_BIT);
glLoadIdentity();
printf("display1\n");
glFlush();
}

void display2(void)
{
  glutSetWindow(window2);
glClearColor(1.0, 0.0, 0.0, 1.0);
glClear(GL_COLOR_BUFFER_BIT);
glLoadIdentity();
printf("display2\n");
glFlush();
}

void reshape (int w, int h)
{
  glViewport(0,0,(GLsizei)w,(GLsizei)h);
  glutPostRedisplay();
}

void  keyboard1(unsigned char key, int x, int y)
{
  static int hide = 0;

  glutSetWindow(window2);
  if (!hide) {
    glutHideWindow();
    hide = 1;
  } else {
    glutShowWindow();
    reshape(width,height);
    hide = 0;
  }
}
void  keyboard2(unsigned char key, int x, int y)
{
  static int hide = 0;
  
  glutSetWindow(window);
  if (!hide) {
    glutHideWindow();
    hide = 1;
  } else {
    glutShowWindow();
    hide = 0;
    reshape(width,height);
  }
}

int main(int argc, char **argv)
{
// Initialization stuff
glutInit(&argc, argv);
glutInitDisplayMode(GLUT_RGB);
glutInitWindowSize(width, height);

// Make Main outer window
window = glutCreateWindow("Window 1 cyan");
glutDisplayFunc(display);
glutReshapeFunc(reshape);
glutKeyboardFunc(keyboard1);

// Create First subwindow
window2 = glutCreateWindow("Window 2 red");
glutDisplayFunc(display2);
glutReshapeFunc(reshape);
glutKeyboardFunc(keyboard2);

// Enter Glut Main Loop and wait for events
glutMainLoop();
return 0;
}

