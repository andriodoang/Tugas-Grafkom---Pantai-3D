//--OPEN GL--
#include <windows.h> 
#include <GL/glut.h>
#include <stdio.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <stdlib.h>
#include "imageloader.h"
#include "vec3f.h"


GLuint texture[20];

typedef struct Image Image;

#define checkImageWidth 100
#define checkImageHeight 100
GLubyte checkImage[checkImageWidth][checkImageHeight][3];
using namespace std;

float sudutnya = 30;

static GLfloat spin, spin2 = 0.0;
float angle = 0;
float lastx, lasty;
GLint stencilBits;
//--- Scrip Awal Untuk View Objeck--///
static int viewx = 140;
static int viewy = 200;
static int viewz = 360;



float rot = 0;

///--- Clas Untuk Membuat Terain---///
class Terrain {  //class untuk terain
private:
	int w; //--Lebar--\\
	int l; //--Panjang--\\
	float** hs; //--Heights--\\
	Vec3f** normals;  //--Inisialisasi Objeck apabila normal--\\
	bool computedNormals; 
	
public:
	Terrain(int w2, int l2) {
		w = w2;
		l = l2;

		hs = new float*[l];
		for (int i = 0; i < l; i++) {
			hs[i] = new float[w];
		}

		normals = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals[i] = new Vec3f[w];
		}

		computedNormals = false;
	}

	~Terrain() {
		for (int i = 0; i < l; i++) {
			delete[] hs[i];
		}
		delete[] hs;

		for (int i = 0; i < l; i++) {
			delete[] normals[i];
		}
		delete[] normals;
	}

	int width() {
		return w;
	}

	int length() {
		return l;
	}

	//--Mengatur ketinggian Terain di (x, z) ke y--\\
	void setHeight(int x, int z, float y) {
		hs[z][x] = y;
		computedNormals = false;
	}

	//--Mengembalikan ketinggian Pola di (x, z)--\\
	float getHeight(int x, int z) {
		return hs[z][x];
	}

	//--Menghitung  meletakkan semua memory yang dikehendaki untuk kondisi normal-\\
	void computeNormals() {
		if (computedNormals) {
			return;
		}

		//Compute the rough version of the normals
		Vec3f** normals2 = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals2[i] = new Vec3f[w];
		}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum(0.0f, 0.0f, 0.0f);

				Vec3f out;
				if (z > 0) {
					out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
				}
				Vec3f in;
				if (z < l - 1) {
					in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
				}
				Vec3f left;
				if (x > 0) {
					left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
				}
				Vec3f right;
				if (x < w - 1) {
					right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
				}

				if (x > 0 && z > 0) {
					sum += out.cross(left).normalize();
				}
				if (x > 0 && z < l - 1) {
					sum += left.cross(in).normalize();
				}
				if (x < w - 1 && z < l - 1) {
					sum += in.cross(right).normalize();
				}
				if (x < w - 1 && z > 0) {
					sum += right.cross(out).normalize();
				}

				normals2[z][x] = sum;
			}
		}

			//--Objeck Keadaan Normal--//
		const float FALLOUT_RATIO = 0.5f;
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum = normals2[z][x];

				if (x > 0) {
					sum += normals2[z][x - 1] * FALLOUT_RATIO;
				}
				if (x < w - 1) {
					sum += normals2[z][x + 1] * FALLOUT_RATIO;
				}
				if (z > 0) {
					sum += normals2[z - 1][x] * FALLOUT_RATIO;
				}
				if (z < l - 1) {
					sum += normals2[z + 1][x] * FALLOUT_RATIO;
				}

				if (sum.magnitude() == 0) {
					sum = Vec3f(0.0f, 1.0f, 0.0f);
				}
				normals[z][x] = sum;
			}
		}

		for (int i = 0; i < l; i++) {
			delete[] normals2[i];
		}
		delete[] normals2;

		computedNormals = true;
	}

	//-Scrip Kembali Ke Posisi Normal Apabila Objeck Sudah di Posisi (x, z)-/
	Vec3f getNormal(int x, int z) {
		if (!computedNormals) {
			computeNormals();
		}
		return normals[z][x];
	}
};
//---end class---//

//--Fungsi InitReading Di Terain--//

void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
}


//--Fungsi Proses Loader Image--//

int LoadBitmap(char *filename)
{
    FILE * file;
    char temp;
    long i;

    //Gambar BMP
    struct {
      int biWidth;
      int biHeight;
      short int biPlanes;
      unsigned short int biBitCount;
      unsigned char *data;
    } infoheader;

    GLuint num_texture;

// Open the file for reading
    if( (file = fopen(filename, "rb"))==NULL) return (-1); 
 /* start reading width & height */
    fseek(file, 18, SEEK_CUR); 
    fread(&infoheader.biWidth, sizeof(int), 1, file);

    fread(&infoheader.biHeight, sizeof(int), 1, file);

    fread(&infoheader.biPlanes, sizeof(short int), 1, file);
    if (infoheader.biPlanes != 1) {
      printf("Planes from %s is not 1: %u\n", filename, infoheader.biPlanes);
      return 0;
    }

    // read the bpp
    fread(&infoheader.biBitCount, sizeof(unsigned short int), 1, file);
    if (infoheader.biBitCount != 24) {
      printf("Bpp from %s is not 24: %d\n", filename, infoheader.biBitCount);
      return 0;
    }

    fseek(file, 24, SEEK_CUR);

    // read the data
    if(infoheader.biWidth<0){
      infoheader.biWidth = -infoheader.biWidth;
    }
    if(infoheader.biHeight<0){
      infoheader.biHeight = -infoheader.biHeight;
    }
    infoheader.data = (unsigned char *) malloc(infoheader.biWidth * infoheader.biHeight * 3);
    if (infoheader.data == NULL) {
      printf("Error allocating memory for color-corrected image data\n");
      return 0;
    }

    if ((i = fread(infoheader.data, infoheader.biWidth * infoheader.biHeight * 3, 1, file)) != 1) {
      printf("Error reading image data from %s.\n", filename);
      return 0;
    }

    for (i=0; i<(infoheader.biWidth * infoheader.biHeight * 3); i+=3) { // reverse all of the colors. (bgr -> rgb)
      temp = infoheader.data[i];
      infoheader.data[i] = infoheader.data[i+2];
      infoheader.data[i+2] = temp;
    }

    fclose(file); // Closes the file stream

    glGenTextures(1, &num_texture);
    glBindTexture(GL_TEXTURE_2D, num_texture); // Bind the ID texture specified by the 2nd parameter

    // The next commands sets the texture parameters
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // If the u,v coordinates overflow the range 0,1 the image is repeated
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // The magnification function ("linear" produces better results)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); //The minifying function

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Finally we define the 2d texture
    glTexImage2D(GL_TEXTURE_2D, 0, 3, infoheader.biWidth, infoheader.biHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, infoheader.data);

    // And create 2d mipmaps for the minifying function
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, infoheader.biWidth, infoheader.biHeight, GL_RGB, GL_UNSIGNED_BYTE, infoheader.data);
    
    free(infoheader.data); // Free the memory we used to load the texture

    return (num_texture); // Returns the current texture OpenGL ID
}  

//--Membuat efek gambar dengan menggunakan skala, rotasi, translasi , fog,terrain--//

//--Fungsi Proses Pembuatan Terrrain Dan proses inisialisai Terrrain--//
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color = (unsigned char) image->pixels[3*(y 
					* image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

float _angle = 100.0f;
//buat tipe data terain
Terrain* _terrain;
Terrain* _terrainTanah;
Terrain* _terrainAir;

//-- Untuk untuk di display DAN Fungsi mengaktifkan pencahayaan--\\
const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void cleanup() {
	delete _terrain;
	delete _terrainTanah;
}


//untuk di display
void drawSceneTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {
	
	float scale = 500.0f / max(terrain->width() - 1, terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
			-(float) (terrain->length() - 1) / 2);

	glColor3f(r, g, b);
	for (int z = 0; z < terrain->length() - 1; z++) {
        
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < terrain->width(); x++) {
			Vec3f normal = terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z), z);
			normal = terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

}



void awan()
{

glPushMatrix();
glColor3ub(153, 223, 255);
glutSolidSphere(10, 50, 50);
glPopMatrix();
glPushMatrix();
glTranslatef(10,0,1);
glutSolidSphere(5, 20, 20);
glPopMatrix();
glPushMatrix();
glTranslatef(-2,6,-2);
glutSolidSphere(7, 50, 50);
glPopMatrix();
glPushMatrix();
glTranslatef(-10,-3,0);
glutSolidSphere(7, 50, 50);
glPopMatrix();
glPushMatrix();
glTranslatef(6,-2,2);
glutSolidSphere(7, 50, 50);
glPopMatrix();
glEndList(); //untuk menutup glnewlist
}


void awan2()
{

glPushMatrix();
glColor3ub(153, 223, 255);
glutSolidSphere(10, 50, 50);
glPopMatrix();
glPushMatrix();
glTranslatef(10,0,1);
glutSolidSphere(5, 20, 20);
glPopMatrix();
glPushMatrix();
glTranslatef(-2,6,-2);
glutSolidSphere(7, 50, 50);
glPopMatrix();
glPushMatrix();
glTranslatef(-10,-3,0);
glutSolidSphere(7, 50, 50);
glPopMatrix();
glPushMatrix();
glTranslatef(6,-2,2);
glutSolidSphere(7, 50, 50);
glPopMatrix();

glEndList(); //untuk menutup glnewlist
}



void awan3()
{

glPushMatrix();
glColor3ub(153, 223, 255);
glutSolidSphere(10, 50, 50);
glPopMatrix();
glPushMatrix();
glTranslatef(10,0,1);
glutSolidSphere(5, 20, 20);
glPopMatrix();
glPushMatrix();
glTranslatef(-2,6,-2);
glutSolidSphere(7, 50, 50);
glPopMatrix();
glPushMatrix();
glTranslatef(-10,-3,0);
glutSolidSphere(7, 50, 50);
glPopMatrix();
glPushMatrix();
glTranslatef(6,-2,2);
glutSolidSphere(7, 50, 50);
glPopMatrix();
glEndList(); 
//untuk menutup glnewlist
}
 
void matahari()
{

glPushMatrix();
glColor3ub(255, 253, 116);
glutSolidSphere(10, 60, 60);
glPopMatrix();
 
glEndList();
}
void pohon(){
//batang
GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);    

glPushMatrix();
glColor3ub(104,70,14);
glRotatef(270,1,0,0);
gluCylinder(pObj, 4, 0.7, 30, 25, 25);
glPopMatrix();
}

void daun()
{

glPushMatrix();
glColor3ub(18,118,13);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(10,5,1);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(-8,6,-2);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(-10,-3,0);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(6,-2,2);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glEndList(); //untuk menutup glnewlist9
}

void pohon2(){
//batang
GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);    

glPushMatrix();
glColor3ub(104,70,14);
glRotatef(270,1,0,0);
gluCylinder(pObj, 4, 0.7, 30, 25, 25);
glPopMatrix();
}


void daun2()
{

glPushMatrix();
glColor3ub(18,118,13);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(10,5,1);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(-8,6,-2);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(-10,-3,0);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(6,-2,2);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glEndList(); //untuk menutup glnewlist9
}

void pohon3(){
//batang
GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);    

glPushMatrix();
glColor3ub(104,70,14);

glRotatef(270,1,0,0);
gluCylinder(pObj, 4, 0.7, 30, 25, 25);
glPopMatrix();
}

void daun3()
{

glPushMatrix();
glColor3ub(18,118,13);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(10,5,1);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(-8,6,-2);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(-10,-3,0);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(6,-2,2);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glEndList(); //untuk menutup glnewlist9
}


void pohon4(){
//batang
GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);    

glPushMatrix();
glColor3ub(104,70,14);
glRotatef(270,1,0,0);
gluCylinder(pObj, 4, 0.7, 30, 25, 25);
glPopMatrix();
}

void daun4()
{

glPushMatrix();
glColor3ub(18,118,13);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(10,5,1);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(-8,6,-2);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(-10,-3,0);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(6,-2,2);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glEndList(); //untuk menutup glnewlist9
}

void pohon5(){
//batang
GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);    

glPushMatrix();
glColor3ub(104,70,14);
glRotatef(270,1,0,0);
gluCylinder(pObj, 4, 0.7, 30, 25, 25);
glPopMatrix();
}

void daun5()
{

glPushMatrix();
glColor3ub(18,118,13);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(10,5,1);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(-8,6,-2);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(-10,-3,0);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glPushMatrix();
glTranslatef(6,-2,2);
glutSolidSphere(10, 10, 10);
glPopMatrix();
glEndList(); //untuk menutup glnewlist9
}


void bola()
{

glPushMatrix();
glColor3ub(230,255,230);
glutSolidSphere(10, 60, 60);
glPopMatrix();
 
glEndList();
}


void meja()
{
	glPushMatrix();	
	glColor3ub(204,204,153);
	glTranslatef(0, 8, 0);
	glScalef(2, 0.2, 2);
	glutSolidCube(0.8);
	glPopMatrix();
	
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(0,7,0);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
   
}

void meja2()
{
	glPushMatrix();	
	glColor3ub(204,204,153);
	glTranslatef(0, 8, 0);
	glScalef(2, 0.2, 2);
	glutSolidCube(0.8);
	glPopMatrix();
	
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(0,7,0);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
   
}

void kursi()
{
	
	glPushMatrix();	
	glColor3ub(204,204,153);
	glTranslatef(0, 8, 2.7);
	glScalef(2, 2, 0.2);
	glutSolidCube(1.2);
	glRotatef(0,0,0,0);
	glPopMatrix();
	
	glPushMatrix();	
	glColor3ub(204,204,153);
	glTranslatef(0, 7, 0);
	glScalef(2, 0.2, 4);
	glutSolidCube(1.4);
	glPopMatrix();
	
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(-1,5.9,2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(-1,5.9,-2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(1,5.9,2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(1,5.9,-2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
}


void kursi2()
{
	
	glPushMatrix();	
	glColor3ub(204,204,153);
	glTranslatef(0, 8, 2.7);
	glScalef(2, 2, 0.2);
	glutSolidCube(1.2);
	glRotatef(0,0,0,0);
	glPopMatrix();
	
	glPushMatrix();	
	glColor3ub(204,204,153);
	glTranslatef(0, 7, 0);
	glScalef(2, 0.2, 4);
	glutSolidCube(1.4);
	glPopMatrix();
	
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(-1,5.9,2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(-1,5.9,-2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
      glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(1,5.9,2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(1,5.9,-2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
}
void kursi3()
{
	
	glPushMatrix();	
	glColor3ub(204,204,153);
	glTranslatef(0, 8, 2.7);
	glScalef(2, 2, 0.2);
	glutSolidCube(1.2);
	glRotatef(0,0,0,0);
	glPopMatrix();
	
	glPushMatrix();	
	glColor3ub(204,204,153);
	glTranslatef(0, 7, 0);
	glScalef(2, 0.2, 4);
	glutSolidCube(1.4);
	glPopMatrix();
	
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(-1,5.9,2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(-1,5.9,-2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
      glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(1,5.9,2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(1,5.9,-2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
}
void kursi4()
{
	
	glPushMatrix();	
	glColor3ub(204,204,153);
	glTranslatef(0, 8, 2.7);
	glScalef(2, 2, 0.2);
	glutSolidCube(1.2);
	glRotatef(0,0,0,0);
	glPopMatrix();
	
	glPushMatrix();	
	glColor3ub(204,204,153);
	glTranslatef(0, 7, 0);
	glScalef(2, 0.2, 4);
	glutSolidCube(1.4);
	glPopMatrix();
	
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(-1,5.9,2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(-1,5.9,-2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
      glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(1,5.9,2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
   
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(1,5.9,-2);
   glScalef(0.1,2.5,0.1);
   glutSolidCube(1.0);
   glPopMatrix();
}

void kincir()
{
     GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);  

   	glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(-2,7,-10);
   glRotatef(260,1,0,0);
   gluCylinder(pObj, 1, 0.5, 25, 25, 25);
   glPopMatrix();    
   
   glPushMatrix();
    glTranslatef(-3, 33,-10);
    glRotatef(sudutnya,1,0,0);
    glColor3ub(204,204,153);
    glScalef(1, 1, 20);
    glutSolidCube(0.8);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-3, 33,-10);
    glRotatef(sudutnya,1,0,0);
    glColor3ub(204,204,153);
    glScalef(1, 20, 1);
    glutSolidCube(0.8);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-3, 33,-10);
    glColor3ub(204,204,153);
    glScalef(3, 1, 1);
    glutSolidCube(0.8);
    glPopMatrix();
	
}
void putar(int value)
{
    sudutnya += 5;
    if (sudutnya > 360){
        sudutnya -= 360;
    }

    glutPostRedisplay();
    glutTimerFunc(25, putar, 0);
}


void lapanganvoli()
{
     GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);  

   	glPushMatrix();	
	glColor3ub(204,204,153);
	glTranslatef(0, 27, -38);
	glScalef(0.2, 3, 14);
	glutSolidCube(4);
	glRotatef(120,0,0,100);
	glPopMatrix();
	
	glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(0,7,-10);
   glRotatef(260,1,0,0);
   gluCylinder(pObj, 1, 0.5, 25, 25, 25);
   glPopMatrix();              
             
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(0,7,-60);
   glRotatef(260,1,0,0);
   gluCylinder(pObj, 1, 0.5, 25, 25, 25);
   glPopMatrix();      
}

void payung()
{
     GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);  
	glPushMatrix();	
	glColor3ub(204,100,153);
	glTranslatef(0, 32, 0);
	glScalef(15, 0.2, 15);
	glutSolidSphere(1, 20, 6);
	glPopMatrix();
	
   glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(0,7,0);
   glRotatef(270,1,0,0);
   gluCylinder(pObj, 1, 0.5, 25, 25, 25);
  
   glPopMatrix();
   
}

void payung2()
{
     GLUquadricObj *pObj;
pObj =gluNewQuadric();
gluQuadricNormals(pObj, GLU_SMOOTH);  
	glPushMatrix();	
	glColor3ub(204,100,153);
	glTranslatef(0, 32, 0);
	glScalef(15, 0.2, 15);
	glutSolidSphere(1, 20, 6);
	glPopMatrix();
	
	glPushMatrix();
   glColor3ub(204,204,153);
   glTranslatef(0,7,0);
   glRotatef(270,1,0,0);
   gluCylinder(pObj, 1, 0.5, 25, 25, 25);
  
   glPopMatrix();
   
}
void batu()
{

glPushMatrix();
glColor3ub(112,112,112);
glutSolidSphere(40, 100, 50);
glPopMatrix();

glEndList();
}

void batu2()
{

glPushMatrix();
glColor3ub(112,112,112);
glutSolidSphere(40, 100, 50);
glPopMatrix();

glEndList();
}

void batu3()
{

glPushMatrix();
glColor3ub(112,112,112);
glutSolidSphere(40, 100, 50);
glPopMatrix();
 
glEndList();
}

unsigned int LoadTextureFromBmpFile(char *filename);

void display(void) {
	glClearStencil(0); //clear the stencil buffer
	glClearDepth(1.0);//-Scrip untuk menghindari polygon yang bertumpuk--\
	glClearColor(0.0, 0.8, 0.9, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear the buffers
	glLoadIdentity();//-- inisialisasi matrik modelview Objeck--\\
	gluLookAt(viewx, viewy, viewz, 0.0, 0.0, 5.0, 0.0, 1.0, 0.0);//Scirp Pengecekan sudut pandang pada objek yang terlihat--\

	glPushMatrix();
	drawSceneTanah(_terrain, 0.8, 0.5, 0.2);
	glPopMatrix();

	glPushMatrix();
	drawSceneTanah(_terrainTanah, 0.7f, 0.2f, 0.1f);
	glPopMatrix();

	glPushMatrix();
	drawSceneTanah(_terrainAir, 0.0f, 0.4f, 0.7f);
	glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-30,50,-130);
    awan();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-140,65,-150);
    glScalef(2.0,2.0,2.0);
    awan2();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(40,85,-200);
    glScalef(2.0,2.0,2.0);
    awan3();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(70,-20,150);    
    glScalef(2.0, 2.0, 2.0);
    pohon();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-200,-1,180);    
    glScalef(1.0, 1.0, 1.0);
    pohon2();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-200,-1,140);    
    glScalef(1.0, 1.0, 1.0);
    pohon3();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-200,-1,90);    
    glScalef(1.0, 1.0, 1.0);
    pohon4();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-200,-1,50);    
    glScalef(1.0, 1.0, 1.0);
    pohon5();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-32,-35.2,115);    
    glScalef(5, 5, 5);
    meja();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-120,-35.2,115);    
    glScalef(5, 5, 5);
    meja2();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-20,-35.2,105);    
    glScalef(5, 5, 5);
    kursi();
    glPopMatrix();
    
    
    glPushMatrix();
    glTranslatef(-45,-35.2,105);    
    glScalef(5, 5, 5);
    kursi2();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-108,-35.2,105);    
    glScalef(5, 5, 5);
    kursi3();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-135,-35.2,105);    
    glScalef(5, 5, 5);
    kursi3();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(120,-12,135);    
    glScalef(1, 1, 1);
    lapanganvoli();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(223,-12,135);    
    glScalef(2, 2, 2);
    kincir();
    glPopMatrix();
    
    
    glPushMatrix();
    glTranslatef(-30,-12,110);    
    glScalef(1, 1, 1);
    payung();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-120,-12,110);    
    glScalef(1, 1, 1);
    payung();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-190,40,180);    
    glScalef(1,1,1);
    daun();
    glPopMatrix();
    
        
    glPushMatrix();
    glTranslatef(70,40,150);    
    glScalef(1,1,1);
    daun2();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-195,40,140);    
    glScalef(1,1,1);
    daun3();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-195,40,90);    
    glScalef(1,1,1);
    daun4();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-195,40,50);    
    glScalef(1,1,1);
    daun5();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(100,-3.,90); 
    glScalef(0.5,0.5,0.5);   
    bola();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(70,-10.,160); 
    glScalef(0.11,0.11,0.1);   
    batu();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(80,-10.,150); 
    glScalef(0.11,0.11,0.1);   
    batu2();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(60,-10.,150); 
    glScalef(0.11,0.11,0.1);   
    batu3();
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-25,65,-140);    
    matahari();
    glPopMatrix();
    
  
  

    
     
	glutSwapBuffers();
	glFlush();
	rot++;
	angle++;

}

void init(void) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glDepthFunc(GL_LESS);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);
    
 
    
	_terrain = loadTerrain("heightmap.bmp", 20);
	_terrainTanah = loadTerrain("heightmapTanah.bmp", 20);
	_terrainAir = loadTerrain("heightmapAir.bmp", 20);

	//binding texture

}

static void kibor(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_HOME:
		viewy++;
		break;
	case GLUT_KEY_END:
		viewy--;
		break;
	case GLUT_KEY_UP:
		viewz--;
		break;
	case GLUT_KEY_DOWN:
		viewz++;
		break;

	case GLUT_KEY_RIGHT:
		viewx++;
		break;
	case GLUT_KEY_LEFT:
		viewx--;
		break;

//---mengatur warna cahaya ambient, vektor berisi nilai R,G,B--\\
	case GLUT_KEY_F1: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	case GLUT_KEY_F2: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient2);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse2);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	default:
		break;
	}
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 'd') {

		spin = spin - 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'a') {
		spin = spin + 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'q') {
		viewz++;
	}
	if (key == 'e') {
		viewz--;
	}
	if (key == 's') {
		viewy--;
	}
	if (key == 'w') {
		viewy++;
	}
}
//-Mengatur dan membuat gambar tetap pada posisi yang tepat--//
void reshape(int w, int h) {
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);
}

/* Deklarasi window size, position, dan display mode
* Pemanggilan routin inisialisasi.
* Memanggil fungsi untuk manampilkan objek di layar
*/

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Sample Terain");
	init();

	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(kibor);

	glutKeyboardFunc(keyboard);
	
// mengkonfigurasi sumber cahaya ...
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
	glColorMaterial(GL_FRONT, GL_DIFFUSE);
    glutTimerFunc(25, putar, 0);
	


	glutMainLoop();
	return 0;
}
