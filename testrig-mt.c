#include <stdio.h>
#include <GL/glut.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <math.h>

#define PWM_C
#define PWM_SCALE	9

#include "golomb.h"

/* hide clash */
#define rand __rand_XX
#include <stdlib.h>
#undef rand
#include <string.h>

#define W_WIDTH		500
#define W_HEIGHT	300
#define FROND_WIDTH	100
#define FROND_HEIGHT	50

static int ptr_x, ptr_y;
static int width = W_WIDTH;
static int height = W_HEIGHT;
static int randcol = 0;

enum menuitems {
	m_grid,
	m_smaller,
	m_larger,
	m_samesize,

	m_save,
	m_load,
};

void *SHBSS;

#define E(x,y)	x##y
#define C(a,b)	E(a,b)

extern unsigned char C(GIZ,_init)(void);
extern void C(GIZ,_pix)(unsigned char pix);

unsigned char getpeek(void);

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

#define MAXPEERS	32

struct frond {
	int self;

	int pix;
	int x, y, w, h;
	int poke;
	int peek;

	unsigned char led[16];
	int phasecol[5];

	unsigned char ir_level;
	unsigned char ir_weight;

	struct frond *peers[MAXPEERS];
	int npeers;

	unsigned short	randpool8;
	unsigned int	randpool16;
	unsigned char	*shbss;
};

#define MAXFRONDS	80

static struct frond fronds[MAXFRONDS];
static int nfronds = 0;

#include "libpwm/rand.c"
#include "libpwm/rand16.c"

static struct frond *current = NULL;

static void pokepeers(struct frond *f, int poke);

static int ispeer(struct frond *from, struct frond *to)
{
	int i;

	for(i = 0; i < from->npeers; i++)
		if(from->peers[i] == to)
			return 1;
	return 0;
}

static void makepeer(struct frond *from, struct frond *to)
{
	if (ispeer(from, to))
		return;

	if (from->npeers == MAXPEERS)
		return;

	from->peers[from->npeers++] = to;
}

static void switchto(struct frond *f)
{
	if (f == current)
		return;

	if (current) {
		current->randpool8 = rand_pool;
		current->randpool16 = rand_pool16;
		assert(SHBSS == current->shbss);
	}

	if (f != NULL) {
		rand_pool = f->randpool8;
		rand_pool16 = f->randpool16;
		SHBSS = f->shbss;
	} else
		SHBSS = NULL;

	current = f;
}

static void drawfrond(struct frond *f)
{
	int i;
	glMatrixMode(GL_MODELVIEW);

	//printf("%d: f->x=%d f->y=%d f->w=%d f->h=%d\n", f->self, f->x, f->y, f->w, f->h);
	glLoadIdentity();

	/* connections */
	glColor3f(.5, .5, .5);
	glBegin(GL_LINES);
	for(i = 0; i < f->npeers; i++) {
		struct frond *p = f->peers[i];

		if (ispeer(p, f) && p->self < f->self)
			continue;

		glVertex2i(f->x + f->w/2, f->y + f->h/2);
		glVertex2i(p->x + p->w/2, p->y + p->h/2);
	}
	glEnd();

	/* arrows */
	glColor3f(.75, .75, .5);
	for(i = 0; i < f->npeers; i++) {
		struct frond *p = f->peers[i];
		int fx = f->x+f->w/2;
		int fy = f->y+f->h/2;
		int px = p->x+p->w/2;
		int py = p->y+p->h/2;
		int dx = px - fx;
		int dy = py - fy;
		int tx = fx+dx/3;
		int ty = fy+dy/3;
		float angle = atan2(dy, dx) * 360 / (M_PI*2);

		glPushMatrix();
		glTranslatef(tx, ty, 0);
		glRotatef(angle-90, 0, 0, 1);

		glBegin(GL_TRIANGLES);
		glVertex2i(0, 0);
		glVertex2i(-5, -10);
		glVertex2i( 5, -10);
		glEnd();
		glPopMatrix();
	}

	glTranslatef(f->x, f->y, 0);
	glScalef(f->w, f->h, 1);

	glPushMatrix();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glColor4f(0, 0, 0, .5);
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(1,0);
	glVertex2f(1,1);
	glVertex2f(0,1);
	glEnd();
	glDisable(GL_BLEND);

	glColor3f(.5, .5, .5);
	glBegin(GL_LINE_LOOP);
	glVertex2f(0,0);
	glVertex2f(1,0);
	glVertex2f(1,1);
	glVertex2f(0,1);
	glEnd();

	glScalef(1./200,1./100,1); /* hard-coded 200x100 dimensions */
	glPushMatrix();

	glTranslatef(5, 5, 0);

	glBegin(GL_QUADS);

	if (f->peek) {
		glColor3f(.75, .5, .25);
		glVertex2i(0, 0);
		glVertex2i(5, 0);
		glVertex2i(5, 5);
		glVertex2i(0, 5);
	}
	if (f->poke) {
		glColor3f(.25, .5, .75);
		glVertex2i(0+7, 0);
		glVertex2i(5+7, 0);
		glVertex2i(5+7, 5);
		glVertex2i(0+7, 5);
		f->poke = 0;
	}

	glEnd();
	glPopMatrix();
	
	glScalef(2, 2, 1);
	glTranslatef(10, 10, 0);

	glBegin(GL_QUADS);
	for(i = 0; i < 8; i++) {
		float c = f->led[i] / 16.;
		int ph = i & 1;

		if (i == 0)
			ph = 4;	/* IR */

		glColor3f(c * colours[f->phasecol[ph]][0], 
			  c * colours[f->phasecol[ph]][1],
			  c * colours[f->phasecol[ph]][2]);
		glVertex2i(i * 10, 0);
		glVertex2i(i * 10 + 8, 0);
		glVertex2i(i * 10 + 8, 8);
		glVertex2i(i * 10, 8);
	}
	glEnd();

	glColor3f(0, .5, .5);
	glBegin(GL_QUADS);
	for(i = 0; i < 8; i++) {
		float c = f->led[i] / 16.;

		glVertex2i(i * 10, 1);
		glVertex2i(i * 10 + c * 8, 1);
		glVertex2i(i * 10 + c * 8, 2);
		glVertex2i(i * 10, 2);
	}

	glEnd();


	glTranslatef(0, 20, 0);

	glBegin(GL_QUADS);
	for(i = 0; i < 8; i++) {
		float c = f->led[i+8] / 16.;
		int ph = 2 + (i & 1);

		glColor3f(c * colours[f->phasecol[ph]][0], 
			  c * colours[f->phasecol[ph]][1],
			  c * colours[f->phasecol[ph]][2]);
		glVertex2i((7 - i) * 10, 0);
		glVertex2i((7 - i) * 10 + 8, 0);
		glVertex2i((7 - i) * 10 + 8, 8);
		glVertex2i((7 - i) * 10, 8);
	}
	glEnd();

	glColor3f(0, .5, .5);
	glBegin(GL_QUADS);
	for(i = 0; i < 8; i++) {
		float c = f->led[i+8] / 16.;
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
	glVertex2i(f->ir_level, 15);
	glVertex2i(f->ir_level, 20);
	glVertex2i(0, 20);

	glColor3f(.5, .5, .5);
	glVertex2i(0, 17);
	glVertex2i(f->ir_weight, 17);
	glVertex2i(f->ir_weight, 18);
	glVertex2i(0, 18);

	glEnd();
	glPopMatrix();
	glPopMatrix();
}

static void newfrond(void)
{
	struct frond *frond;
	int sz = C(GIZ,_init)();

	if (nfronds == MAXFRONDS) {
		printf("too many fronds!\n");
		return;
	}

	frond = &fronds[nfronds];

	frond->x = random() % (W_WIDTH - FROND_WIDTH);
	frond->y = random() % (W_HEIGHT - FROND_HEIGHT);
	frond->w = FROND_WIDTH;
	frond->h = FROND_HEIGHT;
	frond->poke = 0;
	frond->pix = 0;
	memset(frond->led, 0x7, sizeof(frond->led));
	frond->self = nfronds;
	memset(frond->peers, 0, sizeof(frond->peers));

	frond->shbss = malloc(sz);
	memset(frond->shbss, 0, sz);

	if (randcol) {
		int i;
		for(i = 0; i < 4; i++)
			frond->phasecol[i] = random() % NCOL;
	}

	do {
		frond->randpool8 = random();
	} while(frond->randpool8 == 0);

	do {
		frond->randpool16 = random();
	} while(frond->randpool16 == 0);

	nfronds++;
}

static void updatefronds(void)
{
	int i;

	for(i = 0; i < nfronds; i++) {
		struct frond *f = &fronds[i];

		switchto(f);

		assert(SHBSS == f->shbss);

		//printf("frond %d pix %d\n", f->self, f->pix);
		memset(f->led, 0, sizeof(f->led));
		C(GIZ,_pix)(f->pix++);

		switchto(NULL);

		assert(current == NULL);
		assert(SHBSS == NULL);

		pokepeers(f, f->poke);
	}

}
		

static struct frond *findfrond(int x, int y)
{
	int i;

	x = x * W_WIDTH / width;
	y = W_HEIGHT - (y * W_HEIGHT / height);

	for(i = 0; i < nfronds; i++) {
		struct frond *f = &fronds[i];

		if (0)
			printf("findfrond(%d, %d): %d (%d, %d) (%d, %d)\n",
			       x, y, i, f->x, f->y, f->x+f->w, f->y+f->h);
		if (x >= f->x && x < (f->x+f->w) &&
		    y >= f->y && y < (f->y+f->h))
			return f;
	}
	return NULL;
}

static void dopoke(struct frond *f, int pokeid)
{
	f->peek++;
}

static void unpoke(struct frond *f, int pokeid)
{
	f->peek--;
}


static void keyup(unsigned char k, int x, int y)
{
	struct frond *f = findfrond(x, y);

	if (f == NULL)
		return;

	switch (k) {
	case 'P':
	case 'p':
		unpoke(f, 0);
		break;
	}
}

static struct frond *drag, *connect;

static void passivemotion(int x, int y)
{
	ptr_x = x * W_WIDTH / width;
	ptr_y = W_HEIGHT - (y * W_HEIGHT / height);
}

static void motion(int x, int y)
{
	int wx = x * W_WIDTH / width;
	int wy = W_HEIGHT - (y * W_HEIGHT / height);

	ptr_x = wx;
	ptr_y = wy;

	if (drag != NULL) {
		int newx = wx - drag->w/2;
		int newy = wy - drag->h/2;
		
		if (newx < 0)
			newx = 0;
		if ((newx+drag->w) >= W_WIDTH)
			newx = W_WIDTH-drag->w;
		if (newy < 0)
			newy = 0;
		if ((newy+drag->h) >= W_HEIGHT)
			newy = W_HEIGHT-drag->h;
		
		drag->x = newx;
		drag->y = newy;
	}
		
}

static void mouse(int button, int state, int x, int y)
{
	struct frond *f = findfrond(x, y);

	if (f != NULL && button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			assert(drag == NULL);
			drag = f;
		} else {
			assert(drag != NULL);
			drag = NULL;
		}
	}

	if (button == GLUT_MIDDLE_BUTTON) {
		if (state == GLUT_DOWN && f != NULL) {
			assert(connect == NULL);
			connect = f;
		} 
		if (state == GLUT_UP) {
			assert(connect != NULL);
			if (f != NULL && f != connect)
				makepeer(connect, f);
			connect = NULL;
		}
	}
}

static void keyboard(unsigned char k, int x, int y)
{
	struct frond *f = findfrond(x, y);

	switch (k) {
	case 27:
		exit(0);
		break;

	case '1':
	case '2':
	case '3':
	case '4':
		if (!f)
			return;

		f->phasecol[k - '1'] = (f->phasecol[k - '1'] + 1) % NCOL;
		break;

	case '=':
	case '+':
		if (!f)
			return;
		f->ir_level += 5;
		break;

	case '-':
	case '_':
		if (!f)
			return;
		f->ir_level -= 5;
		break;

	case 'p':
	case 'P':
		if (!f)
			return;
		dopoke(f, 0);
		break;

	case 'n':
		newfrond();
		break;
	}
}

static int timebase = .01 * 1000;

static void timer(int t)
{
	glutTimerFunc(timebase, timer, 0);

	updatefronds();

	glutPostRedisplay();
}

static void scaleall(float scale)
{
	int i;

	for(i = 0; i < nfronds; i++) {
		struct frond *f = &fronds[i];

		f->w *= scale;
		f->h *= scale;
	}

	glutPostRedisplay();
}

static void arrange_grid()
{
	int x, y, maxy;
	int i;

  again:
	maxy = x = y = 10;
	for(i = 0; i < nfronds; i++) {
		struct frond *f = &fronds[i];

		if (y + f->h > maxy) {
			maxy = y + f->h;
			if (maxy > W_HEIGHT) {
				scaleall(.9);
				goto again;
			}
		}
		if (x != 10 && (x+f->w) > W_WIDTH) {
			x = 10;
			maxy = y = maxy + 10;

		}

		printf("placing %d at (%d, %d)\n",
		       i, x, y);
		f->x = x;
		x += f->w + 10;
		f->y = y;
	}

	glutPostRedisplay();
}

static void setsamesize(void)
{
	int minw = 100000;
	int minh = 100000;
	int i;

	for(i = 0; i < nfronds; i++) {
		struct frond *f = &fronds[i];

		minw = f->w < minw ? f->w : minw;
		minh = f->h < minh ? f->h : minh;
	}

	for(i = 0; i < nfronds; i++) {
		struct frond *f = &fronds[i];

		f->w = minw;
		f->h = minh;
	}

	glutPostRedisplay();
}

static void menu(int v)
{
	switch(v) {
	case m_smaller:
		scaleall(.75);
		break;
	case m_larger:
		scaleall(1.33);
		break;
	case m_samesize:
		setsamesize();
		break;
	}
}

static void fileMenu(int v)
{
}

static void arrangeMenu(int v)
{
	switch(v) {
	case m_grid:
		arrange_grid();
	}
}

static void reshape(int w, int h)
{
	width = w;
	height = h;

	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluOrtho2D (0.0, (GLdouble) W_WIDTH, 0.0, (GLdouble) W_HEIGHT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void pokepeers(struct frond *f, int poke)
{
	int i;

	for(i = 0; i < f->npeers; i++)
		f->peers[i]->poke += poke;
}

static void display()
{
	int i;

	glClearColor(.1, .1, .1, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	for(i = 0; i < nfronds; i++)
		drawfrond(&fronds[i]);

	if (connect) {
		glLoadIdentity();
		glColor3f(1, 1, 1);
		glBegin(GL_LINES);
		glVertex2i(connect->x + connect->w/2, connect->y + connect->h/2);
		glVertex2i(ptr_x, ptr_y);
		glEnd();
	}

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
	char name[100];
	int arg;
	int num = 1;
	int i;
	int arrange, file;

	glutInit(&argc, argv);

	srandom(getpid() + time(NULL));

	while((arg = getopt(argc, argv, "rn:")) != EOF) {
		switch(arg) {
		case 'r':
			randcol = 1;
			break;
		case 'n':
			num = atoi(optarg);
			break;
		}
	}

	for(i = 0; i < num; i++)
		newfrond();		/* initial fronds */

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(W_WIDTH, W_HEIGHT);
	//glutInitWindowPosition(50, 50);
	sprintf(name, "frond");
	glutCreateWindow(name);

	arrange = glutCreateMenu(arrangeMenu);
	glutAddMenuEntry("grid", m_grid);

	file = glutCreateMenu(fileMenu);
	glutAddMenuEntry("save", m_save);
	glutAddMenuEntry("load", m_load);
	
	glutCreateMenu(menu);
	glutAddSubMenu("File", file);
	glutAddSubMenu("Arrange", arrange);
	glutAddMenuEntry("Smaller", m_smaller);
	glutAddMenuEntry("Larger", m_larger);
	glutAddMenuEntry("Same size", m_samesize);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(passivemotion);
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
			current->led[i] = level;
		mask >>= 1;
	}
}

unsigned char ir_input(void)
{
	return current->ir_level;
}

unsigned char ir_avg(void)
{
	unsigned char i = current->ir_level;

	if (i < current->ir_weight) {
		i = current->ir_weight - i;
		if (i < 8)
			i += 7;
		i /= 8;
		current->ir_weight -= i;
	} else {
		i = i - current->ir_weight;
		if (i < 8)
			i += 7;
		i /= 8;
		current->ir_weight += i;
	}

	return current->ir_weight;
}

unsigned char getpeek(void)
{
	int ret = current->peek;

	return ret;
}

void setpoke(unsigned char ch)
{
	current->poke = !!ch;
}
