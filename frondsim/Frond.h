// -*- C++ -*-

#ifndef FROND_H
#define FROND_H

#include <string>
#include <set>
#include <utility>
#include <iostream>

using namespace std;

class Gizmo;
class FrondGroup;

class Frond
{
public:
	typedef pair<float,float> coord_t;

	Frond(int id, const Gizmo *);		// generic constructor
	~Frond();

	void setGizmo(const Gizmo *);

	// reset
	void reset();

	// peer management
	void addPeer(Frond *);
	void delPeer(Frond *);
	bool isPeer(const Frond *) const;
	
	// enable/disable a frond
	void setEnable(bool e) { enabled_ = e; }
	bool isEnabled(void) const;
	
	// our identity
	int getId() const { return self_; }
	
	// IR
	void adjIR(signed char delta) { ir_level_ += delta; }

	// on-screen position; coord is the centre
	void move(const coord_t &coord);

	// where is this frond?
	coord_t getPos() const {
		return coords_;
	}

	// is coord within this frond?
	bool hit(const coord_t &coord) const;

	// one frame step
	void update();

	// draw frond into current GL context
	void draw() const;

	// update our peer's peek status (and our own if keypoke is set)
	void updatePoke(bool keypoke);

	// phase colours
	void incPhaseCol(int phase);

	// save to a file
	void save(ostream &) const;
	
	// load from a file
	void load(FrondGroup *, istream &);

private:
	friend class Gizmo;

	bool	enabled_;		// frond is enabled

	int	self_;			// id
	coord_t	coords_;		// centre coords

	static const float width_ = 50;
	static const float height_ = 25;
	
	unsigned char pix_;
	unsigned char *bss_;

	const Gizmo		*gizmo_;

	typedef set<Frond *>	peerset_t;
	peerset_t		peers_;

	// Frond state
	unsigned short randpool;
	unsigned int randpool16;

	unsigned char leds_[16];	// led state
	int	phasecol_[5];		// colour index

	unsigned char	ir_level_;
	unsigned char	ir_weight_;

	bool poke_;			// we're poking our peers
	bool peek_;			// we're being poked

	void *getBSS() const { return (void *)bss_; }

	unsigned short getRandpool() const
	{
		return randpool; 
	}
	unsigned int getRandpool16() const
	{
		return randpool16; 
	}
	void setRandpool(unsigned short p) { randpool = p; }
	void setRandpool16(unsigned int p) { randpool16 = p; }

	bool getPeek() const { return peek_; }
	void setPoke(bool p) { poke_ = p; }
	void setLEDs(unsigned short mask, unsigned char level);
};

#endif // FROND_H
