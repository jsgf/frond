#include "Frond.h"
#include "FrondGroup.h"
#include "Gizmo.h"

#include <algorithm>
#include <functional>

#include <GL/gl.h>
#include <math.h>

using namespace std;

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

Frond::Frond(int id, const Gizmo *giz)
	: enabled_(false),
	  self_(id),
	  pix_(0),
	  bss_(NULL),
	  gizmo_(NULL),
	  randpool(random()),
	  randpool16(random()),
	  ir_level_(0),
	  ir_weight_(0),
	  poke_(false),
	  peek_(false)
{
	memset(leds_, 0xf, sizeof(leds_));
	memset(phasecol_, 0, sizeof(phasecol_));

	if (giz != NULL)
		setGizmo(giz);
}

Frond::~Frond()
{
	if (bss_ != NULL)
		delete[] bss_;
}

void Frond::save(ostream &out) const
{
	out << getId() << ' ' << coords_.first << ' ' << coords_.second;

	for(peerset_t::const_iterator it = peers_.begin();
	    it != peers_.end();
	    it++) {
		const Frond *f = *it;

		out << ' ' << f->getId();
	}
	out << '\n';
}

void Frond::load(FrondGroup *group, istream &in)
{
	string line;
	float x, y;

	in >> x;
	in >> y;

	move(coord_t(x, y));

	setEnable(true);

	for(;;) {
		int id = -1;

		in >> id;

		if (!in.good() || id == -1) {
			in.clear();
			break;
		}

		Frond *p = group->getFrond(id);

		if (p == NULL) {
			p = new Frond(id, NULL);
			group->addFrond(p);
		}

		addPeer(p);
	}
}

void Frond::setGizmo(const Gizmo *giz)
{
	memset(leds_, 0, sizeof(leds_));
	pix_ = 0;
	
	if (bss_ != NULL) {
		delete[] bss_;
		bss_ = NULL;
	}

	if (giz != NULL) {
		int s = giz->init();
		bss_ = new unsigned char[s];
		memset(bss_, 0, s);
	}
	
	gizmo_ = giz;
}

bool Frond::hit(const coord_t &coord) const {
	if (!isEnabled())
		return false;

	float x = coord.first;
	float y = coord.second;
	float cx = coords_.first;
	float cy = coords_.second;

	return (x >= (cx - width_/2) && x < (cx + width_/2) &&
		y >= (cy - height_/2) && y < (cy + height_/2));
}

void Frond::reset()
{
	const Gizmo *g = gizmo_;
	setGizmo(NULL);
	setGizmo(g);
}

void Frond::addPeer(Frond *peer)
{
	peers_.insert(peer);
}

void Frond::delPeer(Frond *peer)
{
	peers_.erase(peer);
}

bool Frond::isPeer(const Frond *peer) const
{
	return peers_.count((Frond *)peer) != 0;
}

bool Frond::isEnabled(void) const
{
	return enabled_ && gizmo_ != NULL;
}

void Frond::move(const coord_t &coord)
{
	coords_ = coord;
}

void Frond::update()
{
	if (!isEnabled())
		return;

	memset(leds_, 0, sizeof(leds_));
	poke_ = false;		// needs to be set every frame

	gizmo_->pix(pix_++, this);

	peek_ = false;
}

// poke our peers for the next frame
void Frond::updatePoke(bool keypoke)
{
	// we're being poked from the keyboard
	peek_ = peek_ || keypoke;

	// first, poke our peers if update set our poke_
	if (poke_) {
		for(peerset_t::iterator it = peers_.begin();
		    it != peers_.end();
		    it++) {
			Frond *p = *it;

			p->peek_ = true;
		}
	}
}

void Frond::incPhaseCol(int phase)
{
	assert(phase >= 0 && phase < 5);

	phasecol_[phase] = (phasecol_[phase] + 1) % NCOL;
}

void Frond::draw() const
{
	if (!isEnabled())
		return;

	const float cx = coords_.first;
	const float cy = coords_.second;

	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();

	//printf("%d: f->x=%d f->y=%d f->w=%d f->h=%d\n", f->self, f->x, f->y, f->w, f->h);
	glLoadIdentity();

	/* connections */
	glColor3f(.5, .5, .5);
	glBegin(GL_LINES);

	for(peerset_t::iterator it = peers_.begin();
	    it != peers_.end();
	    it++) {
		const Frond *p = *it;

		if (!p->isEnabled()) {
			printf("%d %p not enabled\n", p->getId(), p);
			continue;
		}

		if ((p->getId() < getId()) && p->isPeer(this))
			continue;
		
		coord_t pos = p->getPos();
		float px = pos.first;
		float py = pos.second;

		glVertex2f(cx, cy);
		glVertex2f(px, py);
	}
	glEnd();

	/* arrows */
	glColor3f(.75, .75, .5);
	for(peerset_t::iterator it = peers_.begin();
	    it != peers_.end();
	    it++) {
		const Frond *p = *it;

		if (!p->isEnabled())
			continue;

		float px = p->coords_.first;
		float py = p->coords_.second;
		float dx = px - cx;
		float dy = py - cy;
		float tx = cx + dx/3;
		float ty = cy + dy/3;
		float angle = atan2(dy, dx) * 360 / (M_PI*2);

		glPushMatrix();
		glTranslatef(tx, ty, 0);
		glRotatef(angle-90, 0, 0, 1);

		glBegin(GL_TRIANGLES);
		glVertex2f(0, 0);
		glVertex2f(-5, -10);
		glVertex2f( 5, -10);
		glEnd();
		glPopMatrix();
	}

	glTranslatef(cx - (width_/2), cy - (height_/2), 0);
	glScalef(width_, height_, 1);

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

	if (peek_) {
		glColor3f(.75, .5, .25);
		glVertex2i(0, 0);
		glVertex2i(5, 0);
		glVertex2i(5, 5);
		glVertex2i(0, 5);
	}
	if (poke_) {
		glColor3f(.25, .5, .75);
		glVertex2i(0+7, 0);
		glVertex2i(5+7, 0);
		glVertex2i(5+7, 5);
		glVertex2i(0+7, 5);
	}

	glEnd();
	glPopMatrix();
	
	glScalef(2, 2, 1);
	glTranslatef(10, 10, 0);

	glBegin(GL_QUADS);
	for(int i = 0; i < 8; i++) {
		float c = leds_[i] / 16.;
		int ph = i & 1;

		if (i == 0)
			ph = 4;	/* IR */

		glColor3f(c * colours[phasecol_[ph]][0], 
			  c * colours[phasecol_[ph]][1],
			  c * colours[phasecol_[ph]][2]);
		glVertex2i(i * 10, 0);
		glVertex2i(i * 10 + 8, 0);
		glVertex2i(i * 10 + 8, 8);
		glVertex2i(i * 10, 8);
	}
	glEnd();

	glColor3f(0, .5, .5);
	glBegin(GL_QUADS);
	for(int i = 0; i < 8; i++) {
		float c = leds_[i] / 16.;

		glVertex2f(i * 10, 1);
		glVertex2f(i * 10 + c * 8, 1);
		glVertex2f(i * 10 + c * 8, 2);
		glVertex2f(i * 10, 2);
	}

	glEnd();


	glTranslatef(0, 20, 0);

	glBegin(GL_QUADS);
	for(int i = 0; i < 8; i++) {
		float c = leds_[i+8] / 16.;
		int ph = 2 + (i & 1);

		glColor3f(c * colours[phasecol_[ph]][0], 
			  c * colours[phasecol_[ph]][1],
			  c * colours[phasecol_[ph]][2]);
		glVertex2i((7 - i) * 10, 0);
		glVertex2i((7 - i) * 10 + 8, 0);
		glVertex2i((7 - i) * 10 + 8, 8);
		glVertex2i((7 - i) * 10, 8);
	}
	glEnd();

	glColor3f(0, .5, .5);
	glBegin(GL_QUADS);
	for(int i = 0; i < 8; i++) {
		float c = leds_[i+8] / 16.;
		int x = 7 - i;

		glVertex2f(x * 10, 1);
		glVertex2f(x * 10 + c * 8, 1);
		glVertex2f(x * 10 + c * 8, 2);
		glVertex2f(x * 10, 2);
	}

	glEnd();

	glScalef(80./256, 1, 1);

	glBegin(GL_LINES);
	glColor3f(1, 1, 1);

	for(int i = 0; i <= 256; i += 16) {
		glVertex2i(i, 13);
		glVertex2i(i, 22);
	}
	glEnd();

	glBegin(GL_QUADS);

	glColor3f(.25, .25, .25);
	glVertex2i(0, 15);
	glVertex2i(ir_level_, 15);
	glVertex2i(ir_level_, 20);
	glVertex2i(0, 20);

	glColor3f(.5, .5, .5);
	glVertex2i(0, 17);
	glVertex2i(ir_weight_, 17);
	glVertex2i(ir_weight_, 18);
	glVertex2i(0, 18);

	glEnd();
	glPopMatrix();
	glPopMatrix();
	glPopMatrix();

}


void Frond::setLEDs(unsigned short mask, unsigned char level)
{
	for(int i = 0; mask != 0; i++, mask >>= 1)
		if (mask & 1)
			leds_[i] = level;
}

bool Frond::getPeek() const
{
	return peek_;
}

void Frond::setPoke(bool p)
{
	poke_ = p;
}
