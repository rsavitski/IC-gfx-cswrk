#include <GL/glut.h>


#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iostream>

// Macros
#define VTK_FILE_PATH "./data/face.vtk"

//using namespace std;

/* Using precompiled OpenGL lists for rendering since the geometry never changes,
	very efficient as the data is cached on the GPU memory and is redrawn with one
	function call */


/* TODO: kill. Scratch area

TODO: parametrise all dimensionalities in macros?

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

*/


// GLUT callbacks
void init();
void display();
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);

// Other prototypes
void loadData();
void cleanup();	// memory cleanup registered with atexit()

// Globals
GLuint facelist = 0;	// display list handle

GLfloat *vertCoordArr = NULL;	// (xyz)[]
GLfloat *vertNormArr = NULL;	// (Nx, Ny, Nz)[]
GLfloat *vertTexArr = NULL;		// (Tx, Ty)[]
GLuint *polyArr = NULL; 		// (vertex indices for triangles)[] // Since assuming triangle poly data

long int vertNum = 0;
long int polyNum = 0;

/////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	// Load vertex, poly and tex data into memory
	loadData();
	
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

	// Attach cleanup function to process termination
	atexit(cleanup);

	// Start rendering 
	glutMainLoop();
}

/////////////////////////////////////////////////////////////////////////

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
			for (int i=0; i<polyNum*3; ++i)
			{
				float col = (float)(rand()%100)/100;
				glColor3f(col,col,col);
				//glColor3f(1,1,0);
				GLuint vIdx = polyArr[i];
				glVertex3f(vertCoordArr[vIdx*3], vertCoordArr[vIdx*3+1], vertCoordArr[vIdx*3+2]);
			}
		glEnd();
	glEndList();
	
	/*
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
	glEndList();*/
}

/////////////////////////////////////////////////////////////////////////

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// call precompiled display list
	glCallList(facelist);

	glutSwapBuffers();
}

/////////////////////////////////////////////////////////////////////////

void reshape(int w, int h)
{
	glViewport (0, 0, (GLsizei) w, (GLsizei) h); 

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, 1, 0.2, 10);
	
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.4, 0, -0.02,	// camera pos
			  0, -0.1, 0,	// look at pos
			  0, 1, 0);	// up vector
	
}

/////////////////////////////////////////////////////////////////////////

void keyboard(unsigned char key, int x, int y)
{
	switch (key) 
	{
		case 27: // ESC
			exit(0);
			break;
	}
}

/////////////////////////////////////////////////////////////////////////

void loadData()
{
	printf("Loading data\n"); //TODO: kill

	// Read VTK file for vertex, poly and tex data
	FILE *vtk_fdesc = fopen(VTK_FILE_PATH, "r");
	if (vtk_fdesc == NULL)
	{
		fprintf(stderr, "Can't open VTK at: %s\n", VTK_FILE_PATH);
		exit(-1);
	}

	// misc buffer variables for reading data, getline calls might resize both as needed
	char *buf = NULL;
	if (!(buf = (char*)malloc(1024))) { fprintf(stderr, "malloc failed\n"); exit(-1); }
	size_t bufsize = 1024;


	// Consume VTK header, 4 lines
	getline(&buf, &bufsize, vtk_fdesc); // Note: getline is POSIX.1-2008, available since libc 4.6.27
	getline(&buf, &bufsize, vtk_fdesc); 
	getline(&buf, &bufsize, vtk_fdesc); 
	getline(&buf, &bufsize, vtk_fdesc); 


	// Vertex data
	vertNum = 0;
	fscanf(vtk_fdesc, "%s %ld %s", buf, &vertNum, buf);

	// global vertex coordinate, normal, texture arrays malloc
	if (!(vertCoordArr = (GLfloat*)malloc(vertNum*3*sizeof(GLfloat)))) { fprintf(stderr, "malloc failed\n"); exit(-1); }
	if (!(vertNormArr  = (GLfloat*)malloc(vertNum*3*sizeof(GLfloat)))) { fprintf(stderr, "malloc failed\n"); exit(-1); }
	if (!(vertTexArr   = (GLfloat*)malloc(vertNum*2*sizeof(GLfloat)))) { fprintf(stderr, "malloc failed\n"); exit(-1); }

	// parse data
	for(int i=0; i<vertNum*3; ++i)
	{
		fscanf(vtk_fdesc, "%f ", &vertCoordArr[i]);
	}
	
	
	// Polygon (triangle in this case) data
	polyNum = 0;
	long int cellNum = 0;
	fscanf(vtk_fdesc, "%s %ld %ld", buf, &polyNum, &cellNum);

	// global polygon array malloc
	if (!(polyArr = (GLuint*)malloc(polyNum*3*sizeof(GLuint)))) { fprintf(stderr, "malloc failed\n"); exit(-1); }
		
	// parse data
	int polysize = 0; // NB: assumed to always be 3 in this exercise (as per data provided), varying poly sizes require dynamic arrays
	for(int i=0; i<cellNum; i+=3) 
	{
		fscanf(vtk_fdesc, "%d ", &polysize);	// throw away
		fscanf(vtk_fdesc, "%u ", &polyArr[i]);
		fscanf(vtk_fdesc, "%u ", &polyArr[i+1]);
		fscanf(vtk_fdesc, "%u ", &polyArr[i+2]);
	}
	/*
	for (int i=0; i<polyNum*3; ++i)
	{
		printf("%u\n", polyArr[i]);
	}
	*/
	
	// TODO:	TEXTURE DATA PROCESSING HERE
	//getline(&buf, &bufsize, vtk_fdesc); 
	//getline(&buf, &bufsize, vtk_fdesc); 
	//printf("%s", buf);

	// temp test
	//fscanf(vtk_fdesc, "%f ", &vertTexArr[2]);

	// cleanup	
	free(buf);
}

/////////////////////////////////////////////////////////////////////////

void cleanup()
{
	// function registered with atexit()
	
	free(vertCoordArr);
	free(vertNormArr);
	free(vertTexArr);
	free(polyArr);
	
	printf("leaving so soon?\n"); //TODO: kill
}
