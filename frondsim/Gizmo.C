
extern "C" {
#define TESTRIG
#include "../libpwm/rand.c"
#include "../libpwm/rand16.c"
}

#define rand __rand__		// prevent namespace clash

#include <dlfcn.h>
#include <iostream>
#include <assert.h>

#include "Gizmo.h"
#include "Frond.h"

#undef rand

using std::string;
using std::cout;

Gizmo::Gizmo(const string &input_path)
	: frond_(NULL)
{
	string path = input_path;
	unsigned slash = path.rfind('/');
	unsigned dot = path.rfind('.');
	string name;

	if (slash != string::npos) {
		if (dot != string::npos && dot > slash)
			name.assign(path, slash+1, dot-slash-1);
		else
			name.assign(path, slash+1, path.length());
	} else {
		if (dot != string::npos)
			name.assign(path, 0, dot);
		else
			name.assign(path);
		path = "./" + path;
	}

	void *dl = dlopen(path.c_str(), RTLD_LAZY);

	if (dl == NULL)
		cout << "Falied to open gizmo " << path << ": " << dlerror() << "\n";
	else {
		init_ = (unsigned char (*)(void))dlsym(dl, (name + "_init").c_str());
		pix_ = (void (*)(unsigned char))dlsym(dl, (name + "_pix").c_str());

		if (init_ == NULL || pix_ == NULL) {
			cout << name << " doesn't have " << name << "_init and/or " << name << "_pix\n";
			dlclose(dl);
			dl = NULL;
		}
	}

	dlhandle_ = dl;
}

Gizmo::~Gizmo()
{
	if (dlhandle_ != NULL)
		dlclose(dlhandle_);
}

int Gizmo::init(void) const
{
	return (init_)();
}

static Frond *frond;

unsigned char Gizmo::getpeek() 
{
	return frond->getPeek();
}

void Gizmo::setpoke(unsigned char p)
{
	frond->setPoke(!!p);
}

void Gizmo::set_led(unsigned short mask, unsigned char level)
{
	frond->setLEDs(mask, level);
}

extern "C" {
	void *SHBSS;		// visible to gizmo module
	
	void setpoke(unsigned char p) {
		Gizmo::setpoke(p);
	}

	unsigned char getpeek(void) {
		return Gizmo::getpeek();
	}

	unsigned char ir_avg(void) {
		return 0;
	}

	unsigned char ir_input(void) {
		return 0;
	}

	void set_led(unsigned short mask, unsigned char level) {
		Gizmo::set_led(mask, level);
	}
}


void Gizmo::pix(unsigned char pix, Frond *f) const
{
	assert(frond == NULL);
	assert(SHBSS == NULL);

	frond = f;
	SHBSS = f->getBSS();
	rand_pool = f->getRandpool();
	rand_pool16 = f->getRandpool16();

	(pix_)(pix);

	f->setRandpool(rand_pool);
	f->setRandpool16(rand_pool16);

	frond = NULL;
	SHBSS = NULL;
}
