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
static int prev_x, prev_y;

static bool randcol = false;
static string filename;

static FrondGroup fronds;

static bool bidir = false;	// if true, two-way arrows
static Frond *connect;		// frond being connected from

static bool dragging = false;	// dragging selection

static bool addSelection;	// adding to a selection
static bool selectionrect;	// selection rectangle active
static int sel_x, sel_y;	// selection rectangle corner

static set<Frond *> pokedset;	// which fronds we're poking from the keyboard

// ************************************************************
// GLUT callbacks
// ************************************************************

// -------------------- menus --------------------
enum menuitem {
	m_noop,

	// file
	m_save,
	m_load,

	// edit
	m_selectall,
	m_undo,
	
	// view
	m_zoomin,
	m_zoomout,
	m_recentre,

	// main
	m_bidir,
};

static void mainMenu(int i)
{
	switch(i) {
	case m_bidir:
		bidir = !bidir;
		glutChangeToMenuEntry(4, bidir ? "One-way arrows" : "Two-way arrows", m_bidir);
		break;

	}		
}

static void fileMenu(int i)
{
	switch(i) {
	case m_load:
		fronds.load(filename);
		break;

	case m_save:
		fronds.save(filename);
		break;
	}
}

static void editMenu(int i)
{
	bool update = false;

	switch(i) {
	case m_selectall:
		update = true;
		break;
	}

	if (update)
		glutPostRedisplay();
}

static void viewMenu(int i)
{
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

	if (selectionrect) {
		int x1 = sel_x;// * width / W_WIDTH;
		int y1 = sel_y;// * height / W_HEIGHT;
		int x2 = ptr_x;// * width / W_WIDTH;
		int y2 = ptr_y;// * height / W_HEIGHT;

		glLoadIdentity();
		glColor3f(1, 1, 1);
		glBegin(GL_LINE_LOOP);
		glVertex2i(x1, y1);
		glVertex2i(x2, y1);
		glVertex2i(x2, y2);
		glVertex2i(x1, y2);
		glEnd();
	}
	glutSwapBuffers();
}

static void keyup(unsigned char k, int x, int y)
{
	switch (k) {
	case 'p':
		pokedset.clear();
		break;
	}
}

static void update_ptr(int x, int y)
{
	prev_x = ptr_x;
	prev_y = ptr_y;

	ptr_x = x;
	ptr_y = y;
}

static void passivemotion(int x, int y)
{
	update_ptr(x * W_WIDTH / width,  W_HEIGHT - (y * W_HEIGHT / height));
}

static void do_select(int ax, int ay, int bx, int by)
{
	if (!addSelection)
		fronds.clearSelection();

	fronds.addSelection(Frond::coord_t(ax, ay), Frond::coord_t(bx, by));
}

static void motion(int x, int y)
{
	int wx = x * W_WIDTH / width;
	int wy = W_HEIGHT - (y * W_HEIGHT / height);
	bool update = connect != NULL;

	update_ptr(wx, wy);

	if (dragging) {
		// If we're dragging a single frond or group, move
		// everything around together
		FrondGroup::Frondset_t::iterator it = fronds.firstSelected();
		FrondGroup::Frondset_t::iterator end = fronds.lastSelected();

		for(; it != end; it++) {
			Frond *f = *it;
			Frond::coord_t pos = f->getPos();

			pos.first += ptr_x - prev_x;
			pos.second += ptr_y - prev_y;

			f->move(pos);
		}

		update = true;
	}

	if (selectionrect) {
		// See what's currently inside the selection rectangle
		do_select(sel_x, sel_y, ptr_x, ptr_y);
		update = true;
	}

	if (update)
		glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y)
{
	int wx = x * W_WIDTH / width;
	int wy = W_HEIGHT - (y * W_HEIGHT / height);
	Frond *f = fronds.find(wx, wy);
	bool update = false;
	int modifiers = glutGetModifiers();

	update_ptr(wx, wy);

	printf("wx=%d wy=%d frond=%p id %d\n", wx, wy, f, f ? f->getId() : -1);

	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			if (f != NULL) {
				sel_x = wx;
				sel_y = wy;

				if (fronds.isSelected(f)) {
					// If we're clicking on
					// something which is already
					// selected, then we're
					// dragging the group it's
					// part of
					dragging = true;
				} else {
					// If we're clicking on
					// something directly, then we
					// select it and start
					// dragging it around.

					if (!(modifiers & GLUT_ACTIVE_SHIFT))
						fronds.clearSelection();
				
					fronds.addSelection(Frond::coord_t(sel_x, sel_y));
					dragging = true;

					update = true;
				}
			} else {
				// If we're not clicking on anything,
				// we're starting to drag out a
				// selection rectangle.
				selectionrect = true;
				addSelection = modifiers & GLUT_ACTIVE_SHIFT;

				if (!addSelection)
					fronds.clearSelection();

				sel_x = wx;
				sel_y = wy;

				update = true;
			}
		} else {
			// On button release, stop making a
			// rectangle and stop dragging things.
			printf("selectionrect stop\n");
			selectionrect = false;
			dragging = false;

			update = true;
		}
	}

	if (button == GLUT_MIDDLE_BUTTON) {
		// Middle button on a frond means we're starting to
		// create a connection.
		if (state == GLUT_DOWN && f != NULL) {
			assert(connect == NULL);
			connect = f;
		} 

		if (state == GLUT_UP && connect != NULL) {
			// If we release the middle button in a void,
			// then create a new frond there
			if (f == NULL)
				f = fronds.newFrond(wx, wy);

			// Connect the frond to its peer; make the
			// connection bi-directional if that's the
			// current mode
			if (f != NULL && f != connect) {
				connect->addPeer(f);
				if (bidir)
					f->addPeer(connect);
			}

			connect = NULL;

			update = true;
		}
	}

	if (update)
		glutPostRedisplay();
}

static void special(int k, int x, int y)
{
	int wx = x * W_WIDTH / width;
	int wy = W_HEIGHT - (y * W_HEIGHT / height);
	Frond *f = fronds.find(wx, wy);
	bool selected = f && fronds.isSelected(f);

	// if we're pointing at something and it isn't currently
	// selected, then temporarily select it
	// XXX what if other things are selected?
	if (f && !selected)
		fronds.select(f);

	switch(k) {
	}
}

static void keyboard(unsigned char k, int x, int y)
{
	int wx = x * W_WIDTH / width;
	int wy = W_HEIGHT - (y * W_HEIGHT / height);
	Frond *f = fronds.find(wx, wy);
	bool selected = f && fronds.isSelected(f);

	// if we're pointing at something and it isn't currently
	// selected, then temporarily select it
	// XXX what if other things are selected?
	if (f && !selected)
		fronds.select(f);

	FrondGroup::Frondset_t::iterator it = fronds.firstSelected();
	FrondGroup::Frondset_t::iterator end = fronds.lastSelected();

	switch (k) {
	case 127:
	case 8:
		// delete
		for(; it != end; it++)
			fronds.delFrond(*it);
		break;

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
		for(; it != end; it++)
			(*it)->adjIR(5);
		break;

	case '-':
	case '_':
		for(; it != end; it++)
			(*it)->adjIR(-5);
		break;

	case 'p':
		for(; it != end; it++)
			pokedset.insert(*it);
		break;

	case 'r':
		for(; it != end; it++)
			(*it)->reset();
		break;

	case 'n':
		fronds.newFrond(wx, wy);
		break;
	}

	if (f && !selected)
		fronds.unSelect(f);

	glutPostRedisplay();
}

static const int timebase = (int)(.01 * 1000);

static void timer(int t)
{
	glutTimerFunc(timebase, timer, 0);

	fronds.update(pokedset);

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
	bool loadfile = false;

	glutInit(&argc, argv);

	srandom(getpid() + time(NULL));

	while((arg = getopt(argc, argv, "rlf:n:")) != EOF) {
		switch(arg) {
		case 'r':
			randcol = true;
			break;
		case 'n':
			num = atoi(optarg);
			break;
		case 'f':
			filename.assign(optarg);
			break;
		case 'l':
			loadfile = true;
			break;
		default:
			err++;
			break;
		}
	}
	
	if (optind != argc-1 || err) {
		cerr << "Usage: " << argv[0] << " [-rl] [-n num] [-f file] gizmo\n";
		exit(1);
	}

	fronds.setGizmo(new Gizmo(argv[optind]));

	if (loadfile && filename.size() != 0) {
		fronds.load(filename); // load from file
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
	// arrange menu
	int arrange = glutCreateMenu(arrangeMenu);
	glutAddMenuEntry("grid", m_grid);
#endif

	int file = glutCreateMenu(fileMenu);
	if (filename.size() != 0) {
		static char buf[1024];
		sprintf(buf, "(File: %s)", filename.c_str());

		glutAddMenuEntry(buf, m_noop);
		glutAddMenuEntry("Save", m_save);
		glutAddMenuEntry("Load", m_load);
	} else
		glutAddMenuEntry("(No filename set)", m_noop);

	int edit = glutCreateMenu(editMenu);
	glutAddMenuEntry("select all", m_selectall);
	
	int view = glutCreateMenu(viewMenu);
	glutAddMenuEntry("zoom in", m_zoomin);
	glutAddMenuEntry("zoom out", m_zoomout);
	glutAddMenuEntry("recentre", m_recentre);

	glutCreateMenu(mainMenu);
	glutAddSubMenu("File", file);
	glutAddSubMenu("Edit", edit);
	glutAddSubMenu("View", view);
	
	glutAddMenuEntry("Two-way arrows", m_bidir);

#if 0
	glutAddSubMenu("Arrange", arrange);
	glutAddMenuEntry("Smaller", m_smaller);
	glutAddMenuEntry("Larger", m_larger);
	glutAddMenuEntry("Same size", m_samesize);
#endif

	glutAttachMenu(GLUT_RIGHT_BUTTON);

	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(passivemotion);

	glutIgnoreKeyRepeat(1);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(special);

	glutTimerFunc(timebase, timer, timebase);

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);

	atexit(cleanup);

	glutMainLoop();

	return 0;
}
