#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <math.h>
#include <mpi.h>
#include <iostream>


#include "MinChi.h"

using namespace std;


void MinChi::minimize(int debug)
{

	vector<Curve>	interpolated;
	interpolated.resize(num);

	Curve	merged;
	merged.assign(measured.getQ(),measured.getI());

	chi2_best = 9.999999e37;
	init();

	int	rank;
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	while (!done()) {
		step();
		steps++;

/* denormalize c1,c2 */
		float c1 = maps[0].trueC1(c_test[1]);
		float c2 = maps[0].trueC2(c_test[2]);


/* normalize weights (not strictly necessary but ...) */
		float w = 0;
		for (int i=0; i<num; i++) w += w_test[i];
		if (w != 0.) {
			w = 1./w;
			for (int i=0; i<num; i++) w_test[i] *= w;
		}
		else for (int i=0; i<num; i++) w_test[i] = 1./num;

//		if (debug) {
//			cout << "[" << rank << "] ";
//			for (int i=0; i<num; i++) cout << w_test[i] << " ";
//			cout << c1 << " " << c2 << " ";
//		}
	
		for (int i=0; i<num; i++) maps[i].interpolate(c1,c2,interpolated[i]);

		merged.mergeFrom(interpolated,w_test);
		merged.fit(measured,chi2_test,c_test[0]);


		int	type = 'R';

		if (accept()) {
			w_cur = w_test;
			for (int i=0; i<3; i++) { c_cur[i] = c_test[i]; }
			chi2_cur = chi2_test;

			if (chi2_test < chi2_best) {
				w_best = w_test;
				for (int i=0; i<3; i++) { c_best[i] = c_test[i]; }
				chi2_best = chi2_test;
				step_best = steps;
				best_callback();
				//if (debug) cout << "B";
				type = 'B';
			}
			//else if (debug) cout << "A";
			else type = 'A';

		}
		// else if (debug) cout << "R";

		if (syncsteps && steps % syncsteps == 0) synchronize();

		// if (debug) cout << "\tchi2=" << chi2_test << "\tchi=" << sqrt(chi2_test) << "\tc=" << c_cur[0] << endl;
		if (debug) writeTrace(type);
	}
}

float MinChi::c12penalty()
{
	float	p = 0.;

	if (c_test[1] < alpha) p -= log(c_test[1]/alpha);
	if (c_test[2] < alpha) p -= log(c_test[2]/alpha);

	if (c_test[1] > 1.-alpha) p -= log((1.-c_test[1])/alpha);
	if (c_test[2] > 1.-alpha) p -= log((1.-c_test[2])/alpha);

	return	p;
}

FILE* MinChi::openTrace(const char *name)
{
	trace = fopen(name,"w+");
	return trace;
}

void MinChi::writeTrace(int type)
{
	struct {
		float	c1,c2,chi2;
		int32_t	s;
	} s;
	fwrite(&(w_test[0]),sizeof (w_test[0]),num,trace);
	s.c1 = c_test[1];
	s.c2 = c_test[2];
	s.chi2 = chi2_test;
	s.s = type;
	fwrite(&s,sizeof s,1,trace);
}
