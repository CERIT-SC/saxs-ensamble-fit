#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <strings.h>

#include <mpi.h>

#include <iostream>
#include <iomanip>

#include "Curve.h"
#include "C12Map.h"
#include "BruteForce.h"
#include "RandomWalk.h"
#include "MonteCarlo.h"
#include "STunel.h"

#define FLOAT_MAX	9.99E37

// Just test, disable tunelling
#define PURE_MONTE_CARLO

using namespace std;

static void usage(char const *);

int main(int argc, char ** argv)
{

	char	*prefix = "";
	int	num = -1, syncsteps = 0;
	long	maxsteps = 5000;
	bool	debug = false, parsed = false, lazy = false;
	enum { BRUTEFORCE, RANDOMWALK, MONTECARLO, STUNEL }	alg = STUNEL;

	char	*fmeasured, *tprefix = ".";

	MinChi	*min = 0;

	float	alpha = 0.1,beta = 5e-3,gamma = 500;

	MPI_Init(&argc,&argv);

	cout << "# ";
	for (int i=0; i<argc; i++) cout << argv[i] << " ";
	cout << endl;

	int	opt;
	while ((opt = getopt(argc,argv,"n:m:b:da:l:g:s:qy:t:Lp:")) != EOF) switch (opt) {
		case 'n': num = atoi(optarg); break;
		case 'm': fmeasured = optarg; break;
		case 'l': alpha = atof(optarg); break;
		case 'b': beta = atof(optarg); break;
		case 'g': gamma = atof(optarg); break;
		case 's': maxsteps = atol(optarg); break;
		case 'd': debug = true; break;
		case 'a': if (strcasecmp(optarg,"bruteforce") == 0) alg = BRUTEFORCE;
				  else if (strcasecmp(optarg,"randomwalk")==0) alg = RANDOMWALK;
				  else if (strcasecmp(optarg,"montecarlo")==0) alg = MONTECARLO;
				  else if (strcasecmp(optarg,"stunel")==0) alg = STUNEL;
				  else { usage(argv[0]); return 1; }
			  break;
		case 'q': parsed = true; break;
		case 'y': syncsteps = atoi(optarg); break;
		case 't': tprefix = optarg; break;
		case 'L': lazy = true; break;
		case 'p': prefix = optarg; break;
		default: usage(argv[0]); return 1;
	}

	if (num<=0) { usage(argv[0]); return 1; }
	assert(num < 100); /* XXX: hardcoded %02d elsewhere */

/* maximal step length (alltogether, not per dimension) */ 
	alpha /= sqrt(2. + num);

/** MonteCarlo */

/* MC scaling, 5e-3 step up accepted with 10% */
	beta = - log(.1)/beta; 	

/** StochasticTunel */

/* tunnel scaling */
//	gamma = 1;
//

	Curve	measured;

	vector<C12Map>	maps;
	maps.resize(num);

	if (measured.load(fmeasured)) return 1;
	for (int i=0; i<num; i++) {
		char	buf[PATH_MAX];

		if (lazy) {
			snprintf(buf,sizeof buf,"%s%02d.pdb",prefix,i+1);
			if (maps[i].setLazy(buf,fmeasured)) return 1;
			maps[i].setQMax(measured.getQMax());
		}
		else if (parsed) {
			snprintf(buf, sizeof buf, "%s%02d.c12",prefix,i+1);
			if (maps[i].restore(buf)) return 1;
		}
		else {
			snprintf(buf,sizeof buf,"%s%02d/%02d_%%.2f_%%.3f.dat",prefix,i+1,i+1);
			if (maps[i].load(buf)) return 1;
		}
	}

	switch (alg) {
		case BRUTEFORCE:
			BruteForce *b =	new BruteForce(measured,maps);
			b->setStep(.01);
			min = b;
			break;
		case RANDOMWALK: 
			RandomWalk *r = new RandomWalk(measured,maps);
			r->setParam(alpha);
			r->setMaxSteps(maxsteps);
			min = r;
			break;
		case MONTECARLO: 
			MonteCarlo *m = new MonteCarlo(measured,maps);
			m->setParam(alpha,beta);
			m->setMaxSteps(maxsteps);
			min = m;
			break;
		case STUNEL:
			STunel *t = new STunel(measured,maps);
			t->setParam(alpha,beta,gamma);
			t->setMaxSteps(maxsteps);
			min = t;
			break;
		default:
			cerr << "algorithm " << alg << " not implemented" << endl;
			return 1;
	}

	min->setSyncSteps(syncsteps);


	int	rank;

	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	if (debug) {
		char	buf[PATH_MAX];

		snprintf(buf,PATH_MAX,"%s/ensamble-fit_%d.trc",tprefix,rank);
		if (min->openTrace(buf) == NULL) return 1;
	}

	min->minimize(debug);

	vector<float> & best = min->getBestW();
	float const *c = min->getBestC();
	float chi2 = min->getBestChi2();
	long step = min->getBestStep();

	if (step > 0) {
		cout << "[" << setw(2) << rank << "] best: #" << setw(5) << step << ": " << fixed;
		cout.precision(3);

		for (int i=0; i<num; i++) 
			if (best[i] >= 0.001)  cout << best[i] << " ";
			else cout << "      ";

		cout << c[1] << " " << c[2];
		cout << "\tchi2=" << chi2 << "\tchi=" << sqrt(chi2) << "\tc=" << c[0] << endl;
	}
	else {
		cout << "[" << rank << "] best found by " << -step << endl;
	}

	MPI_Finalize();

}

static void usage(char const *me)
{
	cerr << "usage: " << me << " -n num -m measured " << endl;
}
