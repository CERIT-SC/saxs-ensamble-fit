#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>

#include "C12Map.h"

// #define DEBUG_IPOL

C12Map::C12Map()
{
/* XXX: works fine with defaults, options/config later */

	float	c1range[] = { .95, 1.05 },
		c2range[] = { -2., 4. };
/* real reasonable values */
 	int	c1steps = 21, c2steps = 121;

/* FIXME: just test */
//	c1steps = 5; c2steps = 13;

	setRange(c1range[0],c1range[1],c1steps,
		c2range[0],c2range[1],c2steps);
}

void C12Map::setRange(float min1,float max1,int samples1,
	float min2, float max2, int samples2)
{
	c1min = min1; c2min = min2;
	c1max = max1; c2max = max2;
	c1samples = samples1; c2samples = samples2;

	c1step = (c1max-c1min)/(c1samples-1);
	c2step = (c2max-c2min)/(c2samples-1);

}

int C12Map::load(char const *tmpl)
{
	curves.resize(c1samples);
	for (int ic1 = 0; ic1 < c1samples; ic1++) {
		curves[ic1].resize(c2samples);
		float c1 = c1min + c1step * ic1;
		for (int ic2 = 0; ic2 < c2samples; ic2++) {
			float c2 = c2min + c2step * ic2;

			char	buf[PATH_MAX];
			snprintf(buf,sizeof buf,tmpl,c2,c1);

			if (curves[ic1][ic2].load(buf,false)) return 1;
		}
	}

	return 0;
}

void C12Map::interpolate(float c1, float c2, Curve& out)
{
	int	ic1 = (c1-c1min)/c1step,
		ic2 = (c2-c2min)/c2step;

	float 	wc1 = c1 - c1min - ic1*c1step,
		wc2 = c2 - c2min - ic2*c2step;

	assert(ic1>=0); assert(ic2>=0);
	assert(ic1<c1samples-1); assert(ic2<c2samples-1);

	wc1 /= c1step;
	wc2 /= c2step;

	float	w[2][2] = {
		{ (1.-wc1)*(1.-wc2), (1.-wc1)*wc2 },
		{ wc1*(1.-wc2), wc1*wc2 }
	};

	vector<float> const * oldI[2][2] = {
		{ &curves[ic1][ic2].getI(), &curves[ic1][ic2+1].getI() },
		{ &curves[ic1+1][ic2].getI(), &curves[ic1+1][ic2+1].getI() }
	};

	vector<float>	newI;

	int size = curves[ic1][ic2].getSize();
	newI.resize(size);

	for (int i=0; i<size; i++) {
		newI[i] = w[0][0] * (*oldI[0][0])[i] +
			w[0][1] * (*oldI[0][1])[i] +
			w[1][0] * (*oldI[1][0])[i] +
			w[1][1] * (*oldI[1][1])[i];
	}

#ifdef DEBUG_IPOL
	std::cerr << c1 << " " << c2 << " " << 
		ic1 << " " << ic2 << " " <<
		w[0][0] << " " << 
		w[0][1] << " " << 
		w[1][0] << " " << 
		w[1][1] << std::endl;
#endif
		 

	out.assign(curves[ic1][ic2].getQ(),newI);
}

#define err() { cerr << fname << ": " << strerror(errno) << endl; close (f); return 1; } 
#define truncw() { \
	cerr << fname << ": truncated write" << endl;\
	close(f); return 1;\
}
#define check_write(x) {\
	n = write(f,&(x), sizeof(x));\
	if (n < 0) err(); \
	if (n != sizeof(x)) truncw(); \
}



int C12Map::dump(char const *fname)
{
	int	f = open(fname,O_WRONLY | O_CREAT | O_TRUNC,0644);
	if (f<0) return errno;

	int	n,e;

	check_write(c1min);
	check_write(c1max);
	check_write(c2min);
	check_write(c2max);

	check_write(c1samples);
	check_write(c2samples);

/* c[12]steps recalculated */

	int	s = curves[0][0].getSize();

	check_write(s);
	
	for (int ic1 = 0; ic1 < c1samples; ic1++)
		for (int ic2 = 0; ic2 < c2samples; ic2++) {
			vector<float> const &q = curves[ic1][ic2].getQ();
			int	w = q.size()*sizeof q[0];
			n = write(f,&q[0],w);
			if (n < 0) err();
			if (n != w) truncw();

			vector<float> const &I = curves[ic1][ic2].getI();
			n = write(f,&I[0],w);
			if (n < 0) err();
			if (n != w) truncw();
			
			/* XXX: no experimetal data, don't store errors */
		}
	if (close(f)) { err(); }
	else return 0;
}

#define truncr() { \
	cerr << fname << ": truncated write" << endl;\
	close(f); return 1;\
}

#define check_read(x) {\
	n = read(f,&(x), sizeof(x));\
	if (n < 0) err(); \
	if (n != sizeof(x)) truncr();\
}

int C12Map::restore(char const *fname)
{
	int	f = open(fname,O_RDONLY,0);
	if (f<0) return errno;

	int	n,e;

	float	min1,max1,min2,max2;
	int	samples1,samples2,s;

	check_read(min1);
	check_read(max1);
	check_read(min2);
	check_read(max2);

	check_read(samples1);
	check_read(samples2);

	setRange(min1,max1,samples1,min2,max2,samples2);

	check_read(s);
	vector<float>	q,I;

	q.resize(s);
	I.resize(s);
	int	r = s*sizeof q[0];

	curves.resize(c1samples);
	for (int ic1 = 0; ic1 < c1samples; ic1++) {
		curves[ic1].resize(c2samples);

		for (int ic2 = 0; ic2 < c2samples; ic2++) {
			n = read(f,&q[0],r);
			if (n < 0) err();
			if (n != r) truncr();

			n = read(f,&I[0],r);
			if (n < 0) err();
			if (n != r) truncr();

			curves[ic1][ic2].assign(q,I);
		}
	}

	close(f);
	return 0;
}


#if 0
void C12Map::alignScale(Curve const &ref)
{
	float	qmin, qmax;
	int	steps;

	ref.getScale(qmin,qmax,steps);

	for (int ic1 = 0; ic1 < c1steps; ic1++)
		for (int ic2 = 0; ic2 < c2steps; ic2++)
			if (! curves[ic1][ic2].checkScale(ref)) 
				curves[ic1][ic2].setScale(qmin,qmax,steps);
}

#endif


