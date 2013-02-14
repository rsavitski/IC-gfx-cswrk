#include <GL/glut.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>


// Macros
#define VTK_FILE_PATH "./data/face.vtk"


/* Using precompiled OpenGL lists for rendering since the geometry never changes,
	very efficient as the data is cached on the GPU memory and is redrawn with one
	function call */


//c99 with c++ comments, display lists

/* TODO: kill. Scratch area


NB: think about normals, can't just sum, need to normalise at each step? (otherwise dominated by longer crossproducts)

TODO list:
- camera control
- textures

array for tex data, iterate over it as above
*/


// GLUT callbacks
void init(void);
void display(void);
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
void idle(void);

// Other prototypes
void loadData(void);
void cleanup(void);	// memory cleanup registered with atexit()

// Utility
void cross3f(GLfloat v1[3], GLfloat v2[3], GLfloat* out);
void triangleNormal_unnormalised(GLfloat pt1[3], GLfloat pt2[3], GLfloat pt3[3], GLfloat* out);

/////////////////////////////////////////////////////////////////////////

// Globals
GLuint facelist = 0;	// display list handle

GLfloat *vertCoordArr = NULL;	// (xyz)[]
GLfloat *vertNormArr = NULL;	// (Nx, Ny, Nz)[]
GLfloat *vertTexArr = NULL;		// (Tx, Ty)[]
GLuint *polyArr = NULL; 		// (vertex indices for triangles)[] // Since assuming triangle poly data

long int vertNum = 0;
long int polyNum = 0;

// bounding box and midpt of loaded mesh
GLfloat xyzMinBB[3] = { +INFINITY, +INFINITY, +INFINITY };
GLfloat xyzMaxBB[3] = { -INFINITY, -INFINITY, -INFINITY };
GLfloat meshMidpt[3] = {0,0,0};

// camera position
GLfloat camPos[3] = {0.6 ,0, 0};


//TODO: temp
GLfloat rotangle = 0;
GLfloat light_pos2[] = {2.0, 10.0, 1.0, 1.0};	// position //TODO

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
	glutIdleFunc(idle);

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
	
	//glEnable(GL_NORMALIZE);	// auto-normalisation, not used atm since loadData() normalises all normals properly
	glEnable(GL_DEPTH_TEST);	// Z-buffer

	
	// Enable lighting
	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT0);
	
	// light and material parameters
	GLfloat light_pos[] = {0.0, 2.0, 1.0, 0.0};	// position
	GLfloat light_Ka[] = {0.4, 0.5 ,0.5, 1.0};	// ambient
	GLfloat light_Kd[] = {1.0, 1.0, 1.0, 1.0};	// diffuse
	GLfloat light_Ks[] = {0.5, 0.5, 0.5, 1.0};	// specular

	GLfloat material_Ka[] = {0.4, 0.3, 0.3, 1.0};	// ambient reflectance
	GLfloat material_Kd[] = {0.4, 0.4, 0.3, 1.0};	// diffuse reflectance
	GLfloat material_Ks[] = {0.2, 0.3, 0.3, 1.0};	// specular reflectance
	GLfloat material_Ke[] = {0.0, 0.0, 0.0, 0.0};	// emitted coefficients
	GLfloat material_Se[] = {10};	// specular exponent
	
	// light0 properties
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  light_Ka);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_Kd);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_Ks);

	// material parameters
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material_Ka);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_Kd);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_Ks);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, material_Ke);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_Se);

	
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

	glLoadIdentity();
	
	// <LIGHT>
	// light positions specified here will be relative to view (since specified before view transformation)
	// glLightfv(GL_LIGHT0, GL_POSITION, LIGHTPOS);
	

	// view transformation
	gluLookAt(camPos[0], camPos[1], camPos[2],	// camera position
			  meshMidpt[0], meshMidpt[1], meshMidpt[2],	// look at pt
			  0,1,0);	// up vector


	// <LIGHT>
	// If needed, setting light pos specified here should be relative to the scene (view matrix already applied)	
	// glLightfv(GL_LIGHT0, GL_POSITION, LIGHTPOS);


	glRotatef(rotangle,0.0,1.0,0.0);

	
	// call precompiled display list to display mesh
	glCallList(facelist);

	glutSwapBuffers();
}

/////////////////////////////////////////////////////////////////////////

void reshape(int w, int h)
{
	// viewport sizes
	glViewport (0, 0, (GLsizei) w, (GLsizei) h); 

	// projection
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, 1, 0.2, 10);
		
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
}

/////////////////////////////////////////////////////////////////////////

void keyboard(unsigned char key, int x, int y)
{
	switch (key) 
	{
		case 27: // ESC
			exit(0);
			break;
		case '1':
			// TEMP TODO
			glPushMatrix();
				rotangle += 5;
				glRotatef(rotangle,1.0,0.0,0.0);
				glLightfv(GL_LIGHT0, GL_POSITION, light_pos2);
			glPopMatrix();
			glutPostRedisplay();
	}
}

/////////////////////////////////////////////////////////////////////////

void idle(void)
{
	//glutPostRedisplay();	
}

/////////////////////////////////////////////////////////////////////////

void loadData(void)
{
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


	// Normalise all normals
	for (int i=0; i<vertNum; ++i)
	{
		// k: start of 3-float tuple with Nx,Ny,Nz components
		int k = i*3;

		GLfloat nx = vertNormArr[k];
		GLfloat ny = vertNormArr[k+1];
		GLfloat nz = vertNormArr[k+2];

		GLfloat mag = sqrt(nx*nx + ny*ny + nz*nz);
		
		vertNormArr[k] /= mag;
		vertNormArr[k+1] /= mag;
		vertNormArr[k+2] /= mag;
	}


	// Bounding box calculation of mesh, possible optimisation: move into reading of the data part
	for (int i=0; i<vertNum; ++i)
	{
		// k: start of 3-tuple with x,y,z coordinates of vertex
		int k=i*3;

		// bounding values initialised in the globals section
		for (int j=0; j<3; ++j)
		{
			if (vertCoordArr[k+j] < xyzMinBB[j])
					xyzMinBB[j] = vertCoordArr[k+j];
			else if (vertCoordArr[k+j] > xyzMaxBB[j])
					xyzMaxBB[j] = vertCoordArr[k+j];
		}
	}

	// calculate midpoint of bounding box
	meshMidpt[0] = (xyzMinBB[0]+xyzMaxBB[0])/2;
	meshMidpt[1] = (xyzMinBB[1]+xyzMaxBB[1])/2;
	meshMidpt[2] = (xyzMinBB[2]+xyzMaxBB[2])/2;
	
		
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
