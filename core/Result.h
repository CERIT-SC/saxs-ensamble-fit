#ifndef __RESULT_H
#define __RESULT_H

#include <vector>
#include <list>

using namespace std;

class Result
{
public:
	int	step;
	float	chi2;
	vector<float>	w;
	vector<float>	c;

	Result() { c.resize(3); }

	friend bool operator<(Result const &r1,Result const &r2) { return r1.chi2 < r2.chi2; }
};

class Results
{
	list<Result>	res;
public:
	void	insert(Result &r);
	float	getMinChi2(void);
	void	synchronize(void); 

	int	dump(const char *file,int step,int num);
};


#endif
