// -*- C++ -*-
#ifndef FRONDGROUP_H
#define FRONDGROUP_H

#include <map>

using namespace std;

class Frond;
class Gizmo;

class FrondGroup
{
public:
	FrondGroup();
	~FrondGroup();

	void setGizmo(const Gizmo *g);

	typedef map<int, Frond *> Frondmap_t;

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
	void update(Frond *poke) const;

private:
	Frondmap_t	fronds_;

	const Gizmo	*gizmo_;
	int		id_;
};

#endif // FRONDGROUP_H
