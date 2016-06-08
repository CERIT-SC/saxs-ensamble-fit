#include <limits.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#include <mpi.h>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "Result.h"

float Results::getMinChi2(void)
{
	if (res.size() == 0) return 9.999999e37;
	else return res.begin()->chi2;
}

static bool similar(Result const &r1, Result const &r2)
{
	float	d = 0.;
	int	size = r1.w.size();
	assert(size == r2.w.size());

	for (int i = 0; i<size; i++) {
		float	dd = r1.w[i] - r2.w[i];
		d +=  dd * dd;
	}

	d = sqrtf(d) / size;

	return d < 0.01;
}

void Results::insert(Result &r)
{
	res.push_front(r);
	res.sort();
	res.unique(similar);
}

void Results::synchronize(void)
{
}

int Results::dump(const char *file,int step,int num)
{
	ofstream	f;
	char	tmp[PATH_MAX];
	int	cnt = 0;

	sprintf(tmp,"%s.tmp",file);
	f.open(tmp, ofstream::trunc);
	if (f.fail()) {
		cerr << tmp << ": " << strerror(errno) << endl;
		return 1;	
	}

	f << step << endl;
	for (list<Result>::iterator p = res.begin(); p != res.end(); p++) {
		f << setprecision(2) << scientific << p->c[0] << ',' << setprecision(3);
		f << fixed << p->c[1] << ',' << p->c[2] << ',' << sqrtf(p->chi2);
		for (int i = 0; i < p->w.size(); i++)
			f << "," << p->w[i];
		f << endl;
		if (cnt++ == num) break;
	}

	f.close();
	rename(tmp,file);

	return 0; 
}

