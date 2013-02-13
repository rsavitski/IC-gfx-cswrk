#include <GL/glut.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Macros
#define VTK_FILE_PATH "./data/face.vtk"


/* Using precompiled OpenGL lists for rendering since the geometry never changes,
	very efficient as the data is cached on the GPU memory and is redrawn with one
	function call */
//c99 with c++ comments, display lists

/* TODO: kill. Scratch area

TODO: parametrise all dimensionalities in macros?


once done, iterate over <vertex normal array> and normalise all vectors

array for tex data, iterate over it as above

once done, compile list and gogogo

PS. might want to compute average xyz of vertex coords and the bounding box for setting up camera

*/


// GLUT callbacks
void init(void);
void display(void);
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);

// Other prototypes
void loadData(void);
void cleanup(void);	// memory cleanup registered with atexit()

// Utility
void cross3f(GLfloat v1[3], GLfloat v2[3], GLfloat* out);
void triangleNormal_unnormalised(GLfloat pt1[3], GLfloat pt2[3], GLfloat pt3[3], GLfloat* out);

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

void init(void) 
{	
	glClearColor (0.0, 0.0, 0.0, 0.0);
	
	glShadeModel (GL_SMOOTH);	// interpolation for poly from vertex colors
	
	glEnable(GL_NORMALIZE);		// auto-normalisation, shouldn't affect performance due to display list architecture
	glEnable(GL_DEPTH_TEST);	// Z-buffer
	
	
	// TODO: fix

	GLfloat pos1[] = {0,2,1,0};	
	GLfloat temp[] = {0.4,1,1,1.0};
	GLfloat temp2[] = {0.4,0.8,0.4,1.0};
	GLfloat temp3[] = {1,0.4,0.1,1.0};

	GLfloat shn[] = {10};
	//TODO

	// Enable lighting
	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, pos1);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  temp);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  temp2);
	glLightfv(GL_LIGHT0, GL_SPECULAR, temp3);

	// Set material parameters
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  temp2);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shn);
	
	// create display list (very efficient as the geometry is static)
	facelist = glGenLists(1);

	// compile the display list
	glNewList(facelist, GL_COMPILE);
		for (int i=0; i<polyNum; ++i)
		{
			glBegin(GL_POLYGON);
				// pick the three vertex indices
				GLuint vertIndx1 = polyArr[i*3];
				GLuint vertIndx2 = polyArr[i*3+1];
				GLuint vertIndx3 = polyArr[i*3+2];

				// pick normals and coordinates for all vertices
				glNormal3f(vertNormArr[vertIndx1*3], vertNormArr[vertIndx1*3+1], vertNormArr[vertIndx1*3+2]);
				glVertex3f(vertCoordArr[vertIndx1*3], vertCoordArr[vertIndx1*3+1], vertCoordArr[vertIndx1*3+2]);
				
				glNormal3f(vertNormArr[vertIndx2*3], vertNormArr[vertIndx2*3+1], vertNormArr[vertIndx2*3+2]);
				glVertex3f(vertCoordArr[vertIndx2*3], vertCoordArr[vertIndx2*3+1], vertCoordArr[vertIndx2*3+2]);
				
				glNormal3f(vertNormArr[vertIndx3*3], vertNormArr[vertIndx3*3+1], vertNormArr[vertIndx3*3+2]);
				glVertex3f(vertCoordArr[vertIndx3*3], vertCoordArr[vertIndx3*3+1], vertCoordArr[vertIndx3*3+2]);
			glEnd();
		}
	glEndList();
}

/////////////////////////////////////////////////////////////////////////

void display(void)
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
	glLoadIdentity();	//TODO
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

void loadData(void)
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
	if (!(buf = (char*) malloc(1024))) { fprintf(stderr, "malloc failed\n"); exit(-1); }
	size_t bufsize = 1024;


	// Consume VTK header, 4 lines
	getline(&buf, &bufsize, vtk_fdesc); // Note: getline is POSIX.1-2008, available since libc 4.6.27
	getline(&buf, &bufsize, vtk_fdesc); 
	getline(&buf, &bufsize, vtk_fdesc); 
	getline(&buf, &bufsize, vtk_fdesc); 


	// Vertex data
	vertNum = 0;
	fscanf(vtk_fdesc, "%s %ld %s", buf, &vertNum, buf);

	// global vertex coordinate, normal (zeroed via calloc), texture arrays malloc
	if (!(vertCoordArr = (GLfloat*) malloc(vertNum*3*sizeof(GLfloat)))) { fprintf(stderr, "malloc failed\n"); exit(-1); }
	if (!(vertNormArr  = (GLfloat*) calloc((size_t)vertNum*3, sizeof(GLfloat)))) { fprintf(stderr, "calloc failed\n"); exit(-1); }
	if (!(vertTexArr   = (GLfloat*) malloc(vertNum*2*sizeof(GLfloat)))) { fprintf(stderr, "malloc failed\n"); exit(-1); }

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
	if (!(polyArr = (GLuint*) malloc(polyNum*3*sizeof(GLuint)))) { fprintf(stderr, "malloc failed\n"); exit(-1); }
		
	// parse data
	int polysize = 0; // NB: assumed to always be 3 in this exercise (as per data provided), varying poly sizes require dynamic arrays
	for(int i=0; i<cellNum; i+=3) 
	{
		fscanf(vtk_fdesc, "%d ", &polysize);	// throw away
		fscanf(vtk_fdesc, "%u ", &polyArr[i]);
		fscanf(vtk_fdesc, "%u ", &polyArr[i+1]);
		fscanf(vtk_fdesc, "%u ", &polyArr[i+2]);
	}
	

	// Calculate and sum normals into relevant vertices (not normalised)
	for (int i=0; i<polyNum; ++i)
	{
		GLfloat normal[3] = {0,0,0};
	
		// pick indexes of the three vertices making up the poly	
		GLuint vertIndx1 = polyArr[i*3];
		GLuint vertIndx2 = polyArr[i*3+1];
		GLuint vertIndx3 = polyArr[i*3+2];
		
		// get normal (not normalised)
		triangleNormal_unnormalised(&vertCoordArr[vertIndx1*3], &vertCoordArr[vertIndx2*3], &vertCoordArr[vertIndx3*3], normal);
		
		// add normal components to all relevant vertices (results in a unnormalised average of surface normals at vertices)
		vertNormArr[vertIndx1*3] += normal[0];
		vertNormArr[vertIndx1*3+1] += normal[1];
		vertNormArr[vertIndx1*3+2] += normal[2];
		
		vertNormArr[vertIndx2*3] += normal[0];
		vertNormArr[vertIndx2*3+1] += normal[1];
		vertNormArr[vertIndx2*3+2] += normal[2];
		
		vertNormArr[vertIndx3*3] += normal[0];
		vertNormArr[vertIndx3*3+1] += normal[1];
		vertNormArr[vertIndx3*3+2] += normal[2];
	}
/*
	// TODO: normalise all normals
	for (int i=0; i<vertNum; i++)
	{
		int k = i*3;



		printf("norm: %f ", vertNormArr[k]);
	}*/
	
	
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

void cleanup(void)
{
	// function registered with atexit()
	
	free(vertCoordArr);
	free(vertNormArr);
	free(vertTexArr);
	free(polyArr);
	
	printf("leaving so soon?\n");
}

/////////////////////////////////////////////////////////////////////////

void cross3f(GLfloat v1[3], GLfloat v2[3], GLfloat* out)
{
	// cross product in 3 dimensions
	out[0] = v1[1]*v2[2] - v1[2]*v2[1];
	out[1] = v1[2]*v2[0] - v1[0]*v2[2];
	out[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

/////////////////////////////////////////////////////////////////////////

void triangleNormal_unnormalised(GLfloat pt1[3], GLfloat pt2[3], GLfloat pt3[3], GLfloat* out)
{
	// edge vectors
	GLfloat v1[3] = {0};
	GLfloat v2[3] = {0};

	v1[0] = pt1[0] - pt2[0];
	v1[1] = pt1[1] - pt2[1];
	v1[2] = pt1[2] - pt2[2];

	v2[0] = pt1[0] - pt3[0];
	v2[1] = pt1[1] - pt3[1];
	v2[2] = pt1[2] - pt3[2];

	// calculate cross product (writes over out)
	cross3f(v1,v2,out);
}
