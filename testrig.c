#include <stdio.h>
#include <GL/glut.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

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

unsigned char getpeek(void);

static int width, height;

static int self = -1;
#define MAXPEERS	32
static int peers[MAXPEERS];
static int npeers = 0;

static volatile int *shared = NULL;

static int peek, poke;

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

static void keyup(unsigned char k, int x, int y)
{
	switch (k) {
	case 'P':
	case 'p':
		peek = 0;
		break;
	}
	glutPostRedisplay();
}

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

	case 'p':
	case 'P':
		peek = 1;
		break;
	}

	glutPostRedisplay();
}

static int pix;
static int timebase = .02 * 1000;

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

static void pokepeers(unsigned n)
{
	unsigned int i,m;

	if (!shared)
		return;

	m = 1<<self;

	for(i = 0; i < npeers; i++) {
		int p = shared[peers[i]];
		int op = p;
		p &= ~m;
		p |= (n<<self);
		if (0 && op != p)
			printf("%2d setting %2d %04x->%04x\n", self, peers[i], op, p);
		shared[peers[i]] = p;
	}
}

static void display()
{
	int i;
	static int prevpix = -1;

	glClear(GL_COLOR_BUFFER_BIT);

	if (prevpix != pix) {
		memset(led, 0, sizeof(led));

		C(GIZ,_pix)(pix);
		prevpix = pix;

		pokepeers(poke);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(5, 5, 0);

	glBegin(GL_QUADS);

	if (getpeek()) {
		glColor3f(.75, .5, .25);
		glVertex2i(0, 0);
		glVertex2i(5, 0);
		glVertex2i(5, 5);
		glVertex2i(0, 5);
	}
	if (poke) {
		glColor3f(.25, .5, .75);
		glVertex2i(0+7, 0);
		glVertex2i(5+7, 0);
		glVertex2i(5+7, 5);
		glVertex2i(0+7, 5);
		poke = 0;
	}

	glEnd();
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

#define SHARED "test.shared"

static void cleanshared(void)
{
	pokepeers(0);
	unlink(SHARED);
}

void handler(int sig)
{
	exit(0);
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

#if SEED16 == 0
#define SEED16 ((unsigned int)0xc3a8623d)
#endif

static unsigned int rand_pool16 = SEED16;

unsigned short rand16(void)
{
	unsigned int r = rand_pool16;

	if (r & 1) {
		r >>= 1;
		r ^= 0x6055F65b;
	} else
		r >>= 1;

	rand_pool16 = r;

	return r;
}

int main(int argc, char **argv)
{
	char name[100];
	int sz = C(GIZ,_init)();
	int arg;

	signal(SIGINT, handler);
	signal(SIGTERM, handler);

	SHBSS = malloc(sz);

	glutInit(&argc, argv);

	srandom(getpid() + time(NULL));

	while((arg = getopt(argc, argv, "rs:p:")) != EOF) {
		int i;

		switch(arg) {
		case 's':
			self = atoi(optarg);
			break;
		case 'p':
			if (npeers == MAXPEERS) {
				fprintf(stderr, "too many peers\n");
				exit(1);
			}
			peers[npeers++] = atoi(optarg);
			break;
		case 'r':
			for(i = 0; i < 4; i++)
				phasecol[i] = random() % NCOL;
			break;
		}
	}

	if (self != -1) {
		int fd = open(SHARED, O_RDWR|O_TRUNC|O_CREAT|O_EXCL, 0600);
		if (fd == -1 && errno == EEXIST)
			fd = open(SHARED, O_RDWR, 0600);
		else
			atexit(cleanshared);

		if (fd == -1) {
			perror("can't open " SHARED);
			exit(1);
		}

		ftruncate(fd, sizeof(*shared) * MAXPEERS);
		shared = mmap(NULL, sizeof(*shared) * MAXPEERS, 
			      PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		if (shared == (void *)-1) {
			perror("mmap failed");
			exit(0);
		}
	}

	do {
		rand_pool = random();
	} while(rand_pool == 0);
	do {
		rand_pool16 = random();
	} while(rand_pool16 == 0);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	//glutInitWindowSize(400, 200);
	//glutInitWindowPosition(50, 50);
	sprintf(name, "frond %d", self);
	glutCreateWindow(name);

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutIgnoreKeyRepeat(1);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyup);
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

unsigned char getpeek(void)
{
	int ret = peek;

	if (shared)
		ret |= shared[self] != 0;

	return ret;
}

void setpoke(unsigned char ch)
{
	poke = !!ch;
}
