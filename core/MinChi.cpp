#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <math.h>
#include <mpi.h>
#include <iostream>
#include <fstream>


#include "bobyqa.h"

#include "MinChi.h"

using namespace std;

MinChi *MinChi::inst = 0;

static void normalize(vector<float> & ww, int num)
{
	float w = 0;
	for (int i=0; i<num; i++) w += ww[i];
	if (w != 0.) {
		w = 1./w;
		for (int i=0; i<num; i++) ww[i] *= w;
	}
	else for (int i=0; i<num; i++) ww[i] = 1./num;
}


void MinChi::minimize(int debug)
{
	interpolated.resize(num);
	bool	got_best = false;

	merged.assign(measured.getQ(),measured.getI());
	w_test.resize(num+2);

	init();

	double xu[num+2],xl[num+2];
	for (int i=0; i<num+2; i++) {
		xl[i] = 0.;
		xu[i] = 0.99999;
	}

	int	rank;
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	while (!done()) {
		step();
		steps++;

/* XXX: use tail elements for normalized c1,c2 in eval() */
		w_test[num] = c_test[1];
		w_test[num+1] = c_test[2];

		chi2_test = eval(w_test); // - c12penalty();

#if 0
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
#endif


		int	type = 'R';

		if (accept()) {
			w_cur = w_test;
			for (int i=0; i<3; i++) { c_cur[i] = c_test[i]; }
			chi2_cur = chi2_test;

			float	chi2_best = results.getMinChi2();

			if (!got_best) {
				got_best = true;
				chi2_best = 1.01 * chi2_test;
			}

			if (polish()) {
/* polish the minimum with local optimization */

				double	x[num+2];

				int	n = num+2, npt = 2*n+1, iprint = /* 2 */ 0, maxfun = MinChi::MAX_MIN_STEPS;
				double 	rbeg = alpha, rend = fabs(chi2_test-chi2_best)/1000.; /* XXX: may not work well with randomwalk */

				double	w[(npt+5)*(npt+n)+3*n*(n+5)/2];

				for (int i=0; i<num+2; i++) x[i] = w_test[i];
				bobyqa_(&n,&npt,x,xl,xu,&rbeg,&rend, &iprint,&maxfun,w);
				for (int i=0; i<num+2; i++) w_test[i] = x[i];
				normalize(w_test,num);

				Result	p;
				p.chi2 = eval(w_test);
				p.c[0] = c_test[0]; // XXX: sideeffect in eval() 
				p.c[1] = w_test[num];
				p.c[2] = w_test[num+1];
				p.w = w_test;
				p.w.resize(num);
				p.step = steps;
				results.insert(p);

				best_callback();
				type = 'B';
			}
			else type = 'A';

		}

		if (syncsteps && steps % syncsteps == 0) {
			synchronize();

			if (rank == 0) results.dump("result",steps,10);
		}

		// if (debug) cout << "\tchi2=" << chi2_test << "\tchi=" << sqrt(chi2_test) << "\tc=" << c_cur[0] << endl;
		// XXX: v pripade 'B' zapise taky jen current, tj. odkud se optimalizovalo
		if (debug) writeTrace(type);
	}
	if (rank == 0) results.dump("result",steps,10);
}

float MinChi::eval(vector<float> const & wc)
{
	float	c1 = maps[0].trueC1(wc[num]);
	float	c2 = maps[0].trueC2(wc[num+1]);
	float	chi_out;

/* denormalize c1,c2 */
	vector<float>	ww(num);
	for (int i=0; i<num; i++) ww[i] = wc[i];

/* normalize weights (not strictly necessary but ...) */

	normalize(ww,num);

	for (int i=0; i<num; i++) maps[i].interpolate(c1,c2,interpolated[i]);
	
	merged.mergeFrom(interpolated,ww);
	merged.fit(measured,chi_out,c_test[0]);	// XXX: side effect on c_test

	// return chi_out + c12penalty(wc[num],wc[num+1]);
	return chi_out;
}

float MinChi::c12penalty()
{
	float	p = 0.;

	// XXX: netusim, na co to bylo, ve step() je omezeni
	return 0;

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
	/* FIXME: broken while introducing Results 
	struct {
		float	c1,c2,chi2;
		int32_t	s;
	} s;
	fwrite(&(w_test[0]),sizeof (w_test[0]),num,trace);
	s.c1 = c_test[1];
	s.c2 = c_test[2];
	s.chi2 = type == 'B' ? chi2_best : chi2_test;
	s.s = type;
	fwrite(&s,sizeof s,1,trace);
	*/
}
