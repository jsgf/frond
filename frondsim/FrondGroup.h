// -*- C++ -*-
#ifndef FRONDGROUP_H
#define FRONDGROUP_H

#include <map>
#include <set>

using namespace std;

class Gizmo;

#include "Frond.h"

class FrondGroup
{
public:
	FrondGroup();
	~FrondGroup();

	void setGizmo(const Gizmo *g);

	typedef map<int, Frond *> Frondmap_t;
	typedef set<Frond *> Frondset_t;

	// create a new frond and return its id
	Frond *newFrond(float x, float y);

	// Add a frond to the group. Group then owns frond
	void addFrond(Frond *);

	// Delete a frond from the group; also deleted frond
	void delFrond(Frond *);

	// empty group of all fronds and deletes them all
	void clearFronds();

	// return a Frond given an id
	Frond *getFrond(int id) const;

	// load and save a FrondGroup
	void load(const string &filename);
	void save(const string &filename) const;

	// find a frond by coord
	Frond *find(float x, float y) const;

	// draw fronds
	void draw() const;

	// update fronds
	void update(const set<Frond *> &poked) const;

	// selection handling
	void clearSelection();
	void addSelection(const Frond::coord_t &a);
	void addSelection(const Frond::coord_t &a, const Frond::coord_t &b);
	void setSelection(const Frond::coord_t &a);
	void setSelection(const Frond::coord_t &a, const Frond::coord_t &b);

	bool isSelected(const Frond *) const;

	void select(Frond *);
	void unSelect(Frond *);

	Frondset_t::iterator firstSelected() const { return selected_.begin(); }
	Frondset_t::iterator lastSelected() const { return selected_.end(); }	

private:
	Frondmap_t	fronds_;
	Frondset_t	selected_;

	const Gizmo	*gizmo_;
	int		id_;
};

#endif // FRONDGROUP_H
