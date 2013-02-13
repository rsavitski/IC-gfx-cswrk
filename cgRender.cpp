#include <GL/glut.h>


#include <stdlib.h>

#include <fstream>
#include <iostream>


using namespace std;

/* Using precompiled OpenGL lists for rendering since the geometry never changes,
	very efficient as the data is cached on the GPU memory and is redrawn with one
	function call */

/* TODO: kill. Scratch area

array for each of: coord data, normal data, tex data (2 components)

read vertex n -> malloc all three arrays
read vertex xyz into coord array

array for poly data: (3 tuple of vertice indexes)
read poly n -> malloc poly array
for each poly:
	read vertex xyz's, construct two vectors, get normal, no need to normalise at this stage
	go to each vertex normal array, add the normal components into the appropriate accumulators

once done, iterate over <vertex normal array> and normalise all vectors

array for tex data, iterate over it as above

once done, compile list and gogogo

PS. might want to compute average xyz of vertex coords and the bounding box for setting up camera

(!)
NB:
GLfloat types

*/


GLuint facelist = 0;



void init() 
{	
	glClearColor (0.0, 0.0, 0.0, 0.0);
	
	glShadeModel (GL_SMOOTH);	// interpolation for poly from vertex colors
	
	glEnable(GL_DEPTH_TEST);	// Z-buffer
	
	/*
	// Enable lighting
	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  LightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  LightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, LightSpecular);

	// Set material parameters
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  MaterialSpecular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, MaterialShininess);
	 */

	// create display list (very efficient as the geometry is static)
	facelist = glGenLists(1);

	// compile the display list
	glNewList(facelist, GL_COMPILE);
		glBegin(GL_POLYGON);
			glColor3f(1,1,0);
			glVertex3f(1,1,1);
			glColor3f(0,1,0);
			glVertex3f(0,1,1);
			glColor3f(0,0,1);
			glVertex3f(1,0,1);
		glEnd();
	glEndList();
}

void display(void)
{
	cout << "display" << endl;
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glCallList(facelist);

	glutSwapBuffers();
	/*
	   for (all polygons)
	   glBegin(GL_POLYGON);
	   for (all vertices of polygon)
	// Define texture coordinates of vertex
	glTexCoord2f(...);
	// Define normal of vertex
	glNormal3f(...);
	// Define coordinates of vertex
	glVertex3f(...);
	}
	glEnd();
	}
	glFlush ();
	*/
	 
	}

// called on resizing of window
void reshape (int w, int h)
{
	glViewport (0, 0, (GLsizei) w, (GLsizei) h); 

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90, 1, 0.2, 10);
	
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, 3,	// camera pos
			  0, 0, 0,	// look at pos
			  0, 1, 0);	// up vector
	
}

// called with key events
void keyboard(unsigned char key, int x, int y)
{
	switch (key) 
	{
		case 27: // ESC
			exit(0);
			break;
	}
}

int main(int argc, char** argv)
{
	// Initialize graphics window
	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);	// double buffered

	glutInitWindowSize (768, 768); 
	glutInitWindowPosition (-1, -1);	// window manager determines position
	glutCreateWindow ("GFX");

	// Initialise state
	init();

	// Attach callbacks
	glutDisplayFunc(display); 
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);

	// Start rendering 
	glutMainLoop();
}
