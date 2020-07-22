#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glut.h>

//Window Size
GLsizei winWidth = 1000, winHeight = 800;

//Let's program know the current shape type
//0=none, 1=cube, 2=tetrahedron, 3=icosahedron, etc..
int currentShape = 0;
//Edge size for basic shapes
GLdouble edgeL = 1.0;
//Selects the desired object color and background
GLfloat colorDraw[3] = { 1.0,1.0,1.0 };
GLfloat colorBackground[3] = { 0.0,0.0,0.0 };

bool wireframe = true;

GLUquadricObj* quadric;

//Ellipse variables
float rx = 1.0;
float ry = 2.0;
float rz = 3.0;

//Torus variables
float torusAxial = 2.0;
float torusR = 1.0;

//Data to set up icosehedron shaping
#define X .525731112119133606
#define Z .850650808352039932
#define PI 3.14159265358979323846  // pi

static GLfloat icoVData[12][3] = {
	{-X,0.0,Z},{X,0.0,Z},{-X,0.0,-Z},{X,0.0,-Z},
	{0.0,Z,X},{0.0,Z,-X},{0.0,-Z,X},{0.0,-Z,-X},
	{Z,X,0.0},{-Z,X,0.0},{Z,-X,0.0},{-Z,-X,0}
};

static GLfloat elipseVData[12][3];

static GLuint icoTIndices[20][3] = {
	{1,4,0},{4,9,0},{4,5,9},{8,5,4},{1,8,4},
	{1,10,8},{10,3,8},{8,3,5},{3,2,5},{3,7,2},
	{3,10,7},{10,6,7},{6,11,7},{6,0,11},{6,1,0},
	{10,1,6},{11,0,9},{2,11,9},{5,2,9},{11,2,7}
};

//Initialize and set up lighting
void init(void)
{
	glClearColor(colorBackground[0], colorBackground[1], colorBackground[2],  0.0);

	GLfloat lightPosition[] = { -5.0,1.0,5.0,0.0 };
	GLfloat modelAmbient[] = { 0.5,0.5,0.5,1.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, modelAmbient);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
}

//Clear buffs, setup camera and coloring
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(colorDraw[0],colorDraw[1],colorDraw[2]);
	glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	gluLookAt(5.0, 5.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glScalef(1.0, 1.0, 1.0);

	glFlush();
}

void winReshapeFcn(GLint newWidth, GLint newHeight)
{
	glViewport(0, 0, newWidth, newHeight);

	glMatrixMode(GL_PROJECTION);
	glFrustum(-1.0, 1.0, -1.0, 1.0, 2.0, 20.0);

	glMatrixMode(GL_MODELVIEW);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//Helper function to create normals in subdivide function
void normalize(float v[3])
{
	GLfloat d = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if (d == 0.0) {
		printf("zero length vector");
		return;
	}
	v[0] /= d;
	v[1] /= d;
	v[2] /= d;
}

//Helper to compute cross product for normals
void normCrossProd(float v1[3], float v2[3], float out[3])
{
	out[0] = v1[1] * v2[2] - v1[2] * v2[1];
	out[1] = v1[2] * v2[0] - v1[0] * v2[2];
	out[2] = v1[0] * v2[1] - v1[1] * v2[0];
	normalize(out);
}

//Helper function to draw triangle and set normals
void drawTriangle(float* v1, float* v2, float* v3)
{
	glBegin(GL_TRIANGLES);
	glNormal3fv(v1);
	glVertex3fv(v1);
	glNormal3fv(v2);
	glVertex3fv(v2);
	glNormal3fv(v3);
	glVertex3fv(v3);
	glEnd();
}

//Used to subdivide a polygon's triangles more triangles. Depth variable is number of subdivisions 
void subdivide(float* v1, float* v2, float* v3, long depth)
{
	GLfloat v12[3], v23[3], v31[3];
	GLint i;

	if (depth == 0)
	{
		drawTriangle(v1, v2, v3);
		return;
	}
	for (i = 0; i < 3; i++)
	{
		v12[i] = (v1[i] + v2[i]) / 2.0;
		v23[i] = (v2[i] + v3[i]) / 2.0;
		v31[i] = (v3[i] + v1[i]) / 2.0;
	}
	normalize(v12);
	normalize(v23);
	normalize(v31);

	subdivide(v1, v12, v31, depth - 1);
	subdivide(v2, v23, v12, depth - 1);
	subdivide(v3, v31, v23, depth - 1);
	subdivide(v12, v23, v31, depth - 1);
}

//Function to draw an icosehedron, uses the subdivide function and passes its own depth parameter to it
void drawIco(int depth)
{
	int i;
	for (i = 0; i < 20; i++)
	{
		subdivide(&icoVData[icoTIndices[i][0]][0], &icoVData[icoTIndices[i][1]][0], &icoVData[icoTIndices[i][2]][0], depth);
	}
}

//Function to draw an ellipse using the parametric equation with triangle strips. 
//Concentrics and slices determine definition(num of triangles) for the shape
void DrawEllipsoid(unsigned int concentrics, unsigned int slices, float rx, float ry, float rz)
{
	float tStep = (PI) / (float)slices;
	float sStep = (PI) / (float)concentrics;
	for (float t = -PI / 2; t <= (PI / 2) + .0001; t += tStep)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (float s = -PI; s <= PI + .0001; s += sStep)
		{
			elipseVData[0][0] = rx * cos(t) * cos(s);
			elipseVData[0][1] = ry * cos(t) * sin(s);
			elipseVData[0][2] = rz * sin(t);
			glNormal3fv(&elipseVData[0][0]);
			glVertex3f(rx * cos(t) * cos(s), ry * cos(t) * sin(s), rz * sin(t));
			elipseVData[0][0] = rx * cos(t + tStep) * cos(s);
			elipseVData[0][1] = ry * cos(t + tStep) * sin(s);
			elipseVData[0][2] = rz * sin(t + tStep);
			glNormal3fv(&elipseVData[0][0]);
			glVertex3f(rx * cos(t + tStep) * cos(s), ry * cos(t + tStep) * sin(s), rz * sin(t + tStep));
		}
		glEnd();
	}
}

//Function to draw a torus using the parametric equation with triangle strips. 
//Concentrics and slices determine definition(num of triangles) for the shape
void DrawTorus(unsigned int concentrics, unsigned int slices, float rAxial, float r)
{
	float tStep = (PI) / (float)slices;
	float sStep = (PI) / (float)concentrics;
	for (float t = -PI; t <= PI + .0001; t += tStep)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (float s = -PI; s <= PI + .0001; s += sStep)
		{
			elipseVData[0][0] = (rAxial + r*cos(t)) * cos(s);
			elipseVData[0][1] = (rAxial + r * cos(t)) * sin(s);
			elipseVData[0][2] = r * sin(t);
			glNormal3fv(&elipseVData[0][0]);
			glVertex3f((rAxial + r * cos(t)) * cos(s), (rAxial + r * cos(t)) * sin(s), r * sin(t));
			elipseVData[0][0] = (rAxial + r * cos(t + tStep)) * cos(s);
			elipseVData[0][1] = (rAxial + r * cos(t + tStep)) * sin(s);
			elipseVData[0][2] = r * sin(t + tStep);
			glNormal3fv(&elipseVData[0][0]);
			glVertex3f((rAxial + r * cos(t + tStep)) * cos(s), (rAxial + r * cos(t + tStep)) * sin(s), r * sin(t + tStep));
		}
		glEnd();
	}
}

//Helper function that is used to redraw shapes after the matrix was transformed, or wireframe was triggered
void reDrawCurrent()
{
	if (currentShape == 1)
	{
		if (wireframe == true)
			glutWireCube(edgeL);
		else
			glutSolidCube(edgeL);
	}
	else if (currentShape == 2)
	{
		if (wireframe == true)
			glutWireTetrahedron();
		else
			glutSolidTetrahedron();
	}
	else if (currentShape == 3)
		drawIco(0);
	else if (currentShape == 4)
		drawIco(1);
	else if (currentShape == 5)
		drawIco(2);
	else if (currentShape == 6)
	{
		if (wireframe == true)
			glutWireSphere(1.0, 15, 10);
		else
			glutSolidSphere(1.0, 15, 10);
	}
	else if (currentShape == 7)
	{
		if (wireframe == true)
			glutWireCone(1.0, 2.0, 12, 10);
		else
			glutSolidCone(1.0, 2.0, 12, 10);
	}
	else if (currentShape == 8)
	{
		quadric = gluNewQuadric();
		if (wireframe == true)
		{
			gluQuadricDrawStyle(quadric, GLU_LINE);
			gluQuadricNormals(quadric, GLU_NONE);
		}
		else
		{
			gluQuadricDrawStyle(quadric, GLU_FILL);
			gluQuadricNormals(quadric, GLU_FLAT);
		}
		gluCylinder(quadric, 1.0, 1.0, 3.0, 15, 10);
		gluDeleteQuadric(quadric);
	}
	else if (currentShape == 9)
		DrawEllipsoid(3, 3, rx, ry, rz);
	else if (currentShape == 10)
		DrawEllipsoid(6, 6, rx, ry, rz);
	else if (currentShape == 11)
		DrawEllipsoid(12, 12, rx, ry, rz);
	else if (currentShape == 12)
		DrawTorus(4, 4, torusAxial, torusR);
	else if (currentShape == 13)
		DrawTorus(8, 8, torusAxial, torusR);
	else if (currentShape == 14)
		DrawTorus(12, 12, torusAxial, torusR);
	glFlush();
}

/*
Function handles all normal keyboard input.
'c' clears te screen. 'z' toggles wireframe mode.
Going from left to right on a keyboard, '1'-'0' and 'q'-'r' display the different shapes of the program.
*/
void onKey(unsigned char key, int x, int y)
{
	if (key == 'c')
	{
		currentShape = 0;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glFlush();
	}
	else if (key == 'z')
	{
		if (wireframe == true)
		{
			wireframe = false;
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else
		{
			wireframe = true;
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		reDrawCurrent();
	}
	else if (key == '1')
	{
		currentShape = 1;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (wireframe == true)
			glutWireCube(edgeL);
		else
			glutSolidCube(edgeL);
		glFlush();
	}
	else if (key == '2')
	{
		currentShape = 2;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (wireframe == true)
			glutWireTetrahedron();
		else
			glutSolidTetrahedron();
		glFlush();
	}
	else if (key == '3')
	{
		currentShape = 3;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawIco(0);
		glFlush();
	}
	else if (key == '4')
	{
		currentShape = 4;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawIco(1);
		glFlush();
	}
	else if (key == '5')
	{
		currentShape = 5;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawIco(2);
		glFlush();
	}
	else if (key == '6')
	{
		currentShape = 6;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if(wireframe == true)
			glutWireSphere(1.0,15,10);
		else
			glutSolidSphere(1.0, 15, 10);
		glFlush();
	}
	else if (key == '7')
	{
		currentShape = 7;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if(wireframe == true)
			glutWireCone(1.0, 2.0, 12, 10);
		else
			glutSolidCone(1.0, 2.0, 12, 10);
		glFlush();
	}
	else if (key == '8')
	{
		currentShape = 8;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		quadric = gluNewQuadric();
		if (wireframe == true)
		{
			gluQuadricDrawStyle(quadric, GLU_LINE);
			gluQuadricNormals(quadric, GLU_NONE);
		}
		gluCylinder(quadric, 1.0, 1.0, 3.0, 15, 10);
		gluDeleteQuadric(quadric);
		glFlush();
	}
	else if (key == '9')
	{
		currentShape = 9;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawEllipsoid(3, 3, rx, ry, rz);
		glFlush();
	}
	else if (key == '0')
	{
		currentShape = 10;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawEllipsoid(6, 6, rx, ry, rz);
		glFlush();
	}
	else if (key == 'q')
	{
		currentShape = 11;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawEllipsoid(12, 12, rx, ry, rz);
		glFlush();
	}
	else if (key == 'w')
	{
		currentShape = 12;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawTorus(4, 4, torusAxial, torusR);
		glFlush();
	}
	else if (key == 'e')
	{
		currentShape = 13;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawTorus(8, 8, torusAxial, torusR);
		glFlush();
	}
	else if (key == 'r')
	{
		currentShape = 14;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DrawTorus(12, 12, torusAxial, torusR);
		glFlush();
	}

}

//This function handles arrow key inputs, which will couse the displayed object to rotate on the x and y axis
void onArrow(int key, int x, int y)
{
	if ((currentShape > 0) && (currentShape < 15))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (key == GLUT_KEY_RIGHT)
			glRotatef(1.0, 1.0, 0.0, 0.0);
		else if (key == GLUT_KEY_LEFT)
			glRotatef(-1.0, 1.0, 0.0, 0.0);
		else if (key == GLUT_KEY_UP)
			glRotatef(1.0, 0.0, 1.0, 0.0);
		else if (key == GLUT_KEY_DOWN)
			glRotatef(-1.0, 0.0, 1.0, 0.0);

		reDrawCurrent();
	}
}

//This function handles mouse inputs, which cause the displayed object to scale
//Left click scales up and right click scales down.
void onMouse(int button, int state, int x, int y)
{
	if ((currentShape > 0) && (currentShape < 15))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
			glScalef(1.2, 1.2, 1.2);
		else if(button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
			glScalef(0.8, 0.8, 0.8);
		reDrawCurrent();
	}
}

//The main function of the program, which initializes window, sets overiding glut functions, and starts glut main loop
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(winWidth, winHeight);
	glutCreateWindow("Assignment 2");

	init();
	glutDisplayFunc(display);
	glutReshapeFunc(winReshapeFcn);
	glutMouseFunc(onMouse);
	glutKeyboardFunc(onKey);
	glutSpecialFunc(onArrow);

	glutMainLoop();
	return 0;
}