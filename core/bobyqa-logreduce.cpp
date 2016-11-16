#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

extern "C" {
void dsyevr_ (
	char	*jobz,
	char	*range,
	char	*uplo,
	int	*n,
	double	*a,
	int	*lda,
	double	*vl,
	double	*vu,
	int	*il,
	int	*iu,
	double	*abstol,
	int	*m,
	double	*w,
	double	*z,
	int	*ldz,
	int	*isuppz,
	double	*work,
	int	*lwork,
	int	*iwork,
	int	*liwork,
	int	*info
);
}

using namespace std;

template <class F> class Point;

template <class F> class PointBase {
public:
	F		kappa;
	vector<F>	A,B;

	template <class G> PointBase<F> & operator =(Point<G> const &);
//	template <class G> PointBase<double> & operator =(PointBase<G> const &);
	template <class G> PointBase<F> & operator +=(PointBase<G> const &);

	PointBase<F>(void) {}; 

	PointBase<F>(int s) {
		A.resize(s); 
		B.resize(s);
		for (int i=0; i<s; i++) A[i] = B[i] = 0.;
	}

	template <class G> PointBase<F>(PointBase<G> const & g) {
		kappa = g.kappa;
		for (int i=0; i<g.A.size(); i++) A.push_back(g.A[i]);
		for (int i=0; i<g.B.size(); i++) B.push_back(g.B[i]);
	}

	PointBase<F> & operator *= (double d) {
		kappa *= d;
		for (int i=0; i<A.size(); i++) A[i] *= d;
		for (int i=0; i<B.size(); i++) B[i] *= d;
	}

	F dotp(vector<F> const &v) const {
		F	ret = kappa * v[0];

		for (int i=0; i<A.size(); i++) ret += v[i+1]*A[i];
		for (int i=0; i<B.size(); i++) ret += v[A.size()+i+1]*B[i];

		return	ret;
	}
};

template <class F> template <class G> PointBase<F> & PointBase<F>::operator=(Point<G> const &f)
{
	kappa = f.kappa;
	A = f.A;
	B = f.B;

	return *this;
}

template <class F> template <class G> PointBase<F> & PointBase<F>::operator +=(PointBase<G>  const & r)
{
	kappa += r.kappa;
	for (int i=0; i<A.size(); i++) A[i] += r.A[i];
	for (int i=0; i<B.size(); i++) B[i] += r.B[i];
	return *this;
}

template <class F> class Point: public PointBase<F> {
public:
	F	R,RW,Sp,RMSD,D_avg,D_max;
	vector<string>	types;

	bool	load(istream &);
};


template <class F> bool Point<F>::load(istream & in)
{
	char	line[BUFSIZ];
	this->A.clear();
	this->B.clear();
	this->types.clear();

#define CLUE	"NEWUOA K:"

	while (! in.getline(line,sizeof line).eof() && 
		strncmp(line,CLUE,sizeof(CLUE)-1));

	if (in.eof()) return false;

// NEWUOA K: 0.0746 |  R: -0.0542  R2: 0.0056  RW: -1.3656  Sp: -0.1671  RMSD: 0.6560  D_avg: 0.5653  D_max: 1.7071
	stringstream	parse(line);
	string	s;

	parse >> s >> s >> this->kappa >> s >> s >> R >> s >> s >> s >> RW >> s >> Sp >> s >> RMSD >> s >> D_avg >> s >> D_max;

#define Atomtype "Atom type"
	in.getline(line,sizeof line);
	if (strncmp(line,Atomtype,sizeof(Atomtype)-1)) return false;

	while (! in.getline(line,sizeof line).eof() && strlen(line) > 0) {
		F	a,b;
		stringstream	parse(line);
		parse >> s >> a >> b;
		this->A.push_back(a);
		this->B.push_back(b);
		this->types.push_back(s);
	}

	return !in.eof();
}

class Select {
public:
	float	R2, RMSD;

	Select() {
		R2 = 1.;
		RMSD = 9.999E38;
	}

	bool match(Point<float> const & p) const {
		if (p.R * p.R  > R2) return false;
		if (p.RMSD  > RMSD) return false;

		return true;
	}
};

#include <boost/program_options.hpp>
namespace po = boost::program_options;

int main(int argc, char ** argv)
{

	po::options_description desc("Options");
	desc.add_options()
		("help", "help message")
		("r2", po::value<float>()->default_value(1.0), "select points for dimension reduction - smaller R2 only")
		("rmsd", po::value<float>()->default_value(100000.0), "select points for dimension reduction - smaller RMSD only")
		("dim", po::value<int>()->default_value(2), "reduce to this number of dimensions")
		("all", "map all points, not only those selected by R2/RMSD")
;

	po::variables_map opt;
	try { po::store(po::parse_command_line(argc, argv, desc), opt); }
	catch (exception &e) {
		cerr << argv[0] << ": " << e.what() << endl;
		return 1;
	}
	po::notify(opt);    

	if (opt.count("help")) { cerr << desc; return 1; }

	Select	sel;
	sel.R2 = opt["r2"].as<float>();
	sel.RMSD = opt["rmsd"].as<float>();

	int	dimreduce = opt["dim"].as<int>();
	bool allpoints = opt.count("all");

	vector<Point<float> >	pt;
	Point<float>		p;

	while (p.load(cin)) {
		pt.push_back(p);
		if (pt.size() % 100 == 0) cerr << pt.size() << " points read      \r" << flush;
	}
	cerr << pt.size() << " points read      " << endl;


	PointBase<double> sum(pt[0].A.size());
	vector<PointBase<float> > matched;
	long nmatch = 0;

	for (int i=0; i<pt.size(); i++) 
		if (sel.match(pt[i])) {
			PointBase<float>	pb = pt[i];
			matched.push_back(pb);
			sum += pt[i];
			nmatch++;
		}

	cerr << nmatch << " points match criteria" << endl;

	PointBase<float> minusavg(sum);
	double invnmatch = 1. / nmatch;
        minusavg *= - invnmatch;

	for (int i=0; i<matched.size(); i++) matched[i] += minusavg;

	int	ntypes = pt[0].A.size(), npar = 1 + 2*ntypes,npar2 = npar*npar;
	double *cov = new double[npar2]; // lower triangular, kappa, A1, A2, ..., B1, B2, ...
	vector<double>	par(npar);
	
	for (int k=0; k<npar2; k++) cov[k] = 0.;

	for (int n=0; n < nmatch; n++) {
		par[0] = matched[n].kappa;
		for (int i=0; i<ntypes; i++) {
			par[i+1] = matched[n].A[i];
			par[ntypes+i+1] = matched[n].B[i];
		}

		for (int i=0; i<npar; i++) {
			int	icol = i*npar;
			for (int j=i; j<npar; j++) cov[icol + j] += par[i] * par[j];
		}
	}

	for (int k=0; k<npar2; k++) cov[k] *= invnmatch;

	double zero = 0., abstol = 0. ;
	int	izero = 0,lwork = npar * 100;
	int	m, *isuppz = new int[2*npar], info, liwork = 10*npar, *iwork = new int[liwork];
	double	*w = new double[npar], *z = new double[npar2], *work = new double[lwork];

	dsyevr_("V", // JOBZ
		"A", // RANGE
		"L", // UPLO
		&npar, // N
		cov, // A
		&npar, // LDA
		&zero, // VL
		&zero, // VU
		&izero, // IL
		&izero, // IU
		&abstol, 
		&m, 
		w,
		z,
		&npar, // LDZ
		isuppz,
		work,
		&lwork,
		iwork,
		&liwork,
		&info);

	cerr << "dsyevr() = " << info << endl;
	if (info) return 1;

	vector<float>	evnorm(npar);
	float	evsum = 0, evcum = 0;

	cerr << "raw eigenvalues: " << endl;
	for (int i=0; i<npar; i++) { cerr << w[i] << " "; evsum += w[i]; }
	cerr << endl;

	cerr << "normalized reverse cummulative: " << endl;
	for (int i=0; i<npar; i++) {
		evnorm[i] = w[npar-i-1] / evsum;
		evcum += evnorm[i];
		cerr << evcum << " ";
	}
	cerr << endl;

	vector<vector<float> > evec(dimreduce);

	for (int i=0; i<dimreduce; i++) {
		evec[i].resize(npar);
		int	icol = (npar-i-1) * npar;
		for (int j=0; j<npar; j++) evec[i][j] = z[icol+j];
	}

	for (int i=0; i<pt.size(); i++) 
		if (allpoints || sel.match(pt[i])) {
			for (int j=0; j<dimreduce; j++) {
				cout << pt[i].dotp(evec[j]) << ", ";
			}
			cout << pt[i].RMSD << ", " << pt[i].R*pt[i].R;
			cout << endl;
		}


	delete [] isuppz;
	delete [] iwork;
	delete [] w;
	delete [] z;
	delete [] work;

	return 0;
}

