// -*- C++ -*-
#ifndef GIZMO_H
#define GIZMO_H

#include <string>

using namespace std;

// wrapper class for a compiled Gizmo
class Gizmo
{
public:
	Gizmo(const string &path);
	~Gizmo();

	const string &getName() const;
	const string &getPath() const;

	// gizmo interface functions
	static void setpoke(unsigned char);
	static unsigned char getpeek(void);
	static void set_led(unsigned short mask, unsigned char level);

private:
	string name_;
	string path_;

	void	*dlhandle_;

	friend class Frond;

	Frond *frond_;

	int init() const;
	void pix(unsigned char pix, Frond *f) const;

	unsigned char (*init_)(void);
	void (*pix_)(unsigned char);

};

#endif // GIZMO_H
