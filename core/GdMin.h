#ifndef __GDMIN
#define __GDMIN

#include "MinChi.h"

class GdMin: public MinChi
{
	long	max_steps;
    long max_min_steps; 

	virtual void init(void);

protected:
	virtual void step(void);
	virtual bool done(void) { return steps >= max_steps; }
	virtual bool accept(void) { return true; }
	virtual bool polish(void) { return true; }

public:
	GdMin(Curve &me, vector<C12Map> &ma) : MinChi(me, ma) {};
	void setParam(float a)
	{
		alpha = a;
	}

	void setMaxSteps(long s) { max_steps = s; }
};

#endif
