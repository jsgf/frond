#include <GL/glut.h>
#include <unistd.h>
#include <map>

#include "Frond.h"
#include "FrondGroup.h"
#include "Gizmo.h"

using namespace std;

static const int W_WIDTH = 500;
static const int W_HEIGHT = 300;
static int width = W_WIDTH;
static int height = W_HEIGHT;	// actual window size
static int ptr_x, ptr_y;	// pointer position

static bool randcol = false;
static string filename;

static FrondGroup fronds;

static Frond *drag;		// frond being dragged around
static Frond *connect;		// frond being connected from

// ************************************************************
// GLUT callbacks
// ************************************************************

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

static void display()
{
	glClearColor(.1, .1, .1, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	fronds.draw();

	if (connect) {
		Frond::coord_t pos = connect->getPos();

		glLoadIdentity();
		glColor3f(1, 1, 1);
		glBegin(GL_LINES);
		glVertex2f(pos.first, pos.second);
		glVertex2i(ptr_x, ptr_y);
		glEnd();
	}

	glutSwapBuffers();
}

static Frond *poked = NULL;

static void keyup(unsigned char k, int x, int y)
{
	switch (k) {
	case 'p':
		poked = NULL;
		break;
	}
}

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

	if (drag != NULL)
		drag->move(Frond::coord_t(wx, wy)); // XXX fix offscreen

	if (drag || connect)
		glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y)
{
	int wx = x * W_WIDTH / width;
	int wy = W_HEIGHT - (y * W_HEIGHT / height);
	Frond *f = fronds.find(wx, wy);

	printf("wx=%d wy=%d frond=%p id %d\n", wx, wy, f, f ? f->getId() : -1);

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
		if (state == GLUT_UP && connect != NULL) {
			if (f == NULL)
				f = fronds.newFrond(wx, wy);

			if (f != NULL && f != connect)
				connect->addPeer(f);

			connect = NULL;

			glutPostRedisplay();
		}
	}
}

static void keyboard(unsigned char k, int x, int y)
{
	int wx = x * W_WIDTH / width;
	int wy = W_HEIGHT - (y * W_HEIGHT / height);
	Frond *f = fronds.find(wx, wy);

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

		//f->phasecol[k - '1'] = (f->phasecol[k - '1'] + 1) % NCOL;
		break;

	case '=':
	case '+':
		if (!f)
			return;
		f->adjIR(5);
		break;

	case '-':
	case '_':
		if (!f)
			return;
		f->adjIR(-5);
		break;

	case 'p':
		poked = f;
		break;

	case 'r':
		if (f)
			f->reset();
		break;

	case 'n':
		fronds.newFrond(wx, wy);
		fronds.save(".test");
		break;
	}

	glutPostRedisplay();
}

static const int timebase = (int)(.01 * 1000);

static void timer(int t)
{
	glutTimerFunc(timebase, timer, 0);

	fronds.update(poked);

	glutPostRedisplay();
}



static void cleanup(void)
{
	fronds.clearFronds();
	fronds.setGizmo(NULL);
}

// usage:
// frondsim [-r] [-n num] gizmo

int main(int argc, char **argv)
{
	char name[100];
	int arg;
	int num = 1;
	int i;
	int err = 0;
	string loadfile;

	glutInit(&argc, argv);

	srandom(getpid() + time(NULL));

	while((arg = getopt(argc, argv, "rl:n:")) != EOF) {
		switch(arg) {
		case 'r':
			randcol = true;
			break;
		case 'n':
			num = atoi(optarg);
			break;
		case 'l':
			loadfile.assign(optarg);
			break;

		default:
			err++;
			break;
		}
	}
	
	if (optind != argc-1 || err) {
		cerr << "Usage: " << argv[0] << " [-r] [-n num] [-l loadfile] gizmo\n";
		exit(1);
	}

	fronds.setGizmo(new Gizmo(argv[optind]));

	if (loadfile.size() != 0) {
		fronds.load(loadfile); // load from file
	} else {
		for(i = 0; i < num; i++)
			fronds.newFrond(random()%W_WIDTH, random()%W_HEIGHT);		/* initial fronds */
	}

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(W_WIDTH, W_HEIGHT);
	//glutInitWindowPosition(50, 50);
	sprintf(name, "frond");
	glutCreateWindow(name);

#if 0
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

#endif
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(passivemotion);
	glutIgnoreKeyRepeat(1);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyup);
	glutTimerFunc(timebase, timer, timebase);

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);

	atexit(cleanup);

	glutMainLoop();

	return 0;
}
