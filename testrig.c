#include <stdio.h>
#include <GL/glut.h>

#define PWM_C
#define PWM_SCALE	9

#include "golomb.h"

/* hide clash */
#define rand __rand_XX
#include <stdlib.h>
#undef rand
#include <string.h>

void *SHBSS;

#define E(x,y)	x##y
#define C(a,b)	E(a,b)

static unsigned char led[16];

extern unsigned char C(GIZ,_init)(void);
extern void C(GIZ,_pix)(unsigned char pix);

static const char dens[] = " '.,:;~/+=^!$&*%@#";

static int width, height;

static const float colours[][3] = {
	{ 1, 1, 1 },		/* white */
	{ 1, 0, 0 },		/* red */
	{ 0, 1, 0 },		/* green */
	{ 1, 1, 0 },		/* yellow */
	{ 1, .5, 0 },		/* orange */
	{ 0, 0, 1 },		/* blue */
	{ .8, 0, .8  },		/* IR */
};
#define NCOL	((sizeof(colours)/sizeof(*colours))-1)

static int phasecol[] = { 0, 0, 0, 0, NCOL };

static unsigned char ir_level;

static void keyboard(unsigned char k, int x, int y)
{
	switch (k) {
	case 27:
		exit(0);
		break;

	case '1':
	case '2':
	case '3':
	case '4':
		phasecol[k - '1'] = (phasecol[k - '1'] + 1) % NCOL;
		break;

	case '=':
	case '+':
		ir_level += 5;
		break;

	case '-':
	case '_':
		ir_level -= 5;
		break;
	}

	glutPostRedisplay();
}

static int pix;
static int timebase = .1 * 1000;

static void timer(int t)
{
	glutTimerFunc(timebase, timer, 0);

	pix++;
	glutPostRedisplay();
}


static void reshape(int w, int h)
{
	width = w;
	height = h;

	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluOrtho2D (0.0, (GLdouble) 200, 0.0, (GLdouble) 100);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static unsigned char ir_weight;

static void display()
{
	int i;
	static int prevpix = -1;

	glClear(GL_COLOR_BUFFER_BIT);

	if (prevpix != pix) {
		memset(led, 0, sizeof(led));

		C(GIZ,_pix)(pix);
		prevpix = pix;
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glScalef(2, 2, 1);
	glTranslatef(10, 10, 0);

	glBegin(GL_QUADS);
	for(i = 0; i < 8; i++) {
		float c = led[i] / 16.;
		int ph = i & 1;

		if (i == 0)
			ph = 4;	/* IR */

		glColor3f(c * colours[phasecol[ph]][0], 
			  c * colours[phasecol[ph]][1],
			  c * colours[phasecol[ph]][2]);
		glVertex2i(i * 10, 0);
		glVertex2i(i * 10 + 8, 0);
		glVertex2i(i * 10 + 8, 8);
		glVertex2i(i * 10, 8);
	}
	glEnd();

	glColor3f(0, .5, .5);
	glBegin(GL_QUADS);
	for(i = 0; i < 8; i++) {
		float c = led[i] / 16.;

		glVertex2i(i * 10, 1);
		glVertex2i(i * 10 + c * 8, 1);
		glVertex2i(i * 10 + c * 8, 2);
		glVertex2i(i * 10, 2);
	}
	glEnd();


	glTranslatef(0, 20, 0);

	glBegin(GL_QUADS);
	for(i = 0; i < 8; i++) {
		float c = led[i+8] / 16.;
		int ph = 2 + (i & 1);

		glColor3f(c * colours[phasecol[ph]][0], 
			  c * colours[phasecol[ph]][1],
			  c * colours[phasecol[ph]][2]);
		glVertex2i((7 - i) * 10, 0);
		glVertex2i((7 - i) * 10 + 8, 0);
		glVertex2i((7 - i) * 10 + 8, 8);
		glVertex2i((7 - i) * 10, 8);
	}
	glEnd();

	glColor3f(0, .5, .5);
	glBegin(GL_QUADS);
	for(i = 0; i < 8; i++) {
		float c = led[i+8] / 16.;
		int x = 7 - i;

		glVertex2i(x * 10, 1);
		glVertex2i(x * 10 + c * 8, 1);
		glVertex2i(x * 10 + c * 8, 2);
		glVertex2i(x * 10, 2);
	}
	glEnd();

	glScalef(80./256, 1, 1);

	glBegin(GL_LINES);
	glColor3f(1, 1, 1);

	for(i = 0; i <= 256; i += 16) {
		glVertex2i(i, 13);
		glVertex2i(i, 22);
	}
	glEnd();

	glBegin(GL_QUADS);

	glColor3f(.25, .25, .25);
	glVertex2i(0, 15);
	glVertex2i(ir_level, 15);
	glVertex2i(ir_level, 20);
	glVertex2i(0, 20);

	glColor3f(.5, .5, .5);
	glVertex2i(0, 17);
	glVertex2i(ir_weight, 17);
	glVertex2i(ir_weight, 18);
	glVertex2i(0, 18);
	
	glEnd();

	glutSwapBuffers();
}

unsigned char pwmsec(float s, int pre)
{
	unsigned char t = (s * 12000000) / (PWM_LENGTH * pre);

	return t;
}

float secpwm(unsigned char t, int pre)
{
	float s = (t * (PWM_LENGTH * pre)) / 12000000.;

	return s;
}

void set_framerate(unsigned char fr)
{
	float s = secpwm(fr, 64);

	timebase = s * 1000.;
}

int main(int argc, char **argv)
{
	int sz = C(GIZ,_init)();

	SHBSS = malloc(sz);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(400, 200);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("frond");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutTimerFunc(timebase, timer, timebase);

	glutMainLoop();

	return 0;
}

void set_led(unsigned short mask, unsigned char level)
{
	int i;

	if (level >= 16)
		level = 15;

	for(i = 0; i < 16 && mask; i++) {
		if (mask & 1)
			led[i] = level;
		mask >>= 1;
	}
}

#if SEED == 0
#define SEED 0x8cf5
#endif

static unsigned short rand_pool = SEED;

unsigned char rand(void)
{
	unsigned short r = rand_pool;

	if (r & 1) {
		r >>= 1;
		r ^= 0x8016;
	} else
		r >>= 1;

	rand_pool = r;

	return r;
}

unsigned char ir_input(void)
{
	return ir_level;
}

unsigned char ir_avg(void)
{
	unsigned char i = ir_level;

	if (i < ir_weight) {
		i = ir_weight - i;
		if (i < 8)
			i += 7;
		i /= 8;
		ir_weight -= i;
	} else {
		i = i - ir_weight;
		if (i < 8)
			i += 7;
		i /= 8;
		ir_weight += i;
	}

	return ir_weight;
}

