#include "FrondGroup.h"
#include "Frond.h"
#include "Gizmo.h"

#include <iostream>
#include <fstream>
#include <string>

#include <assert.h>
#include <GL/gl.h>

using namespace std;

FrondGroup::FrondGroup()
	: id_(1)
{
}

FrondGroup::~FrondGroup()
{
}

void FrondGroup::setGizmo(const Gizmo *g)
{
	gizmo_ = g;
}

Frond *FrondGroup::newFrond(float x, float y)
{
	Frond *f = new Frond(id_++, gizmo_);
	f->move(Frond::coord_t(x,y));

	f->setEnable(true);

	addFrond(f);

	return f;
}

void FrondGroup::addFrond(Frond *f)
{
	fronds_[f->getId()] = f;
}

void FrondGroup::delFrond(Frond *f)
{
	assert(fronds_[f->getId()] == f);
	fronds_.erase(f->getId());

	for(Frondmap_t::iterator it = fronds_.begin();
	    it != fronds_.end();
	    it++) {
		Frond *p = it->second;

		p->delPeer(f);
	}

	delete f;
}

void FrondGroup::clearFronds()
{
	for(Frondmap_t::iterator it = fronds_.begin();
	    it != fronds_.end();
	    it++) {
		Frond *f = it->second;

		delete f;
	}

	fronds_.clear();
	id_ = 1;
}

void FrondGroup::save(const string &filename) const
{
	ofstream out;

	out.open(filename.c_str());

	for(Frondmap_t::const_iterator it = fronds_.begin();
	    it != fronds_.end();
	    it++) {
		const Frond *f = it->second;
		out << "frond ";
		f->save(out);
	}

	out.close();
}

void FrondGroup::load(const string &filename)
{
	ifstream in;

	clearFronds();

	in.open(filename.c_str());

	for(;;) {
		string word;

		in >> word;

		if (in.eof())
			break;

		if (word == "gizmo") {
			string giz;

			in >> giz;
			if (in.good()) {
				cout << "New gizmo " << giz << '\n';
				setGizmo(new Gizmo(giz));
			}
		} else if (word == "frond") {
			int id;

			in >> id;

			if (id > id_)
				id_ = id+1;

			cout << "word=" << word << " id=" << id << "\n";

			Frond *f = getFrond(id);

			if (f == NULL) {
				f = new Frond(id, gizmo_);
				addFrond(f);
			} else
				f->setGizmo(gizmo_);

			f->load(this, in);
		}
	} 
}

Frond *FrondGroup::getFrond(int id) const
{
	Frond *ret = NULL;
	Frondmap_t::const_iterator it = fronds_.find(id);
	
	if (it != fronds_.end())
		ret = it->second;

	return ret;
}

Frond *FrondGroup::find(float x, float y) const
{
	Frond::coord_t pos(x,y);

	// go backwards because the top-most ones will be later in the
	// list
	for(Frondmap_t::const_reverse_iterator it = fronds_.rbegin();
	    it != fronds_.rend();
	    it++) {
		Frond *f = it->second;

		if (f->hit(pos))
			return f;
	}

	return NULL;
}

void FrondGroup::draw() const
{
	for(Frondmap_t::const_iterator it = fronds_.begin();
	    it != fronds_.end();
	    it++) {
		const Frond *f = it->second;

		if (isSelected(f)) {
			Frond::coord_t pos = f->getPos();
			float x1 = pos.first - (25 + 5);
			float y1 = pos.second - (12.5 + 5);
			float x2 = pos.first + (25 + 5);
			float y2 = pos.second + (12.5 + 5);
			glColor3f(.25, .25, .25);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glBegin(GL_QUADS);
			glVertex2f(x1, y1);
			glVertex2f(x2, y1);
			glVertex2f(x2, y2);
			glVertex2f(x1, y2);
			glEnd();
		}

		f->draw();
	}
}

void FrondGroup::update(const set<Frond *> &poked) const
{
	// do a frame's worth of work
	for(Frondmap_t::const_iterator it = fronds_.begin();
	    it != fronds_.end();
	    it++) {
		Frond *f = it->second;

		f->update();
	}

	// propagate pokes to peer's peeks
	for(Frondmap_t::const_iterator it = fronds_.begin();
	    it != fronds_.end();
	    it++) {
		Frond *f = it->second;

		f->updatePoke(poked.count(f) != 0);
	}
}


void FrondGroup::clearSelection()
{
	selected_.clear();
}

void FrondGroup::setSelection(const Frond::coord_t &a)
{
	clearSelection();
	addSelection(a);
}

void FrondGroup::addSelection(const Frond::coord_t &a)
{
	Frond *f = find(a.first, a.second);

	if (f != NULL)
		selected_.insert(f);
}

void FrondGroup::addSelection(const Frond::coord_t &a, const Frond::coord_t &b)
{
	for(Frondmap_t::const_iterator it = fronds_.begin();
	    it != fronds_.end();
	    it++) {
		Frond *f = it->second;
		Frond::coord_t p = f->getPos();
		
		if (((p.first >= a.first && p.first < b.first) ||
		     (p.first >= b.first && p.first < a.first)) &&
		    ((p.second >= a.second && p.second < b.second) ||
		     (p.second >= b.second && p.second < a.second))) {
			selected_.insert(f);
		}
	}
}

void FrondGroup::setSelection(const Frond::coord_t &a, const Frond::coord_t &b)
{
	clearSelection();
	addSelection(a, b);
}

bool FrondGroup::isSelected(const Frond *f) const
{
	return selected_.count((Frond *)f) != 0;
}

void FrondGroup::select(Frond *f) 
{
	selected_.insert(f);
}

void FrondGroup::unSelect(Frond *f) 
{
	selected_.erase(f);
}
