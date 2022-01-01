#pragma once
#include "define.h"
class CData
{
private:
	void NormalizeVector(vector <double>* x);
	void NormalizeData(vector <double>* x);

	vector <double> fma;
	vector <double> sma;
	vector <double> mom;
	double _falfa{ 0.0 };
	double _salfa{ 0.0 };
	int _size{ 0 };
	int _period{ 0 };
	int last_period{ 0 };
	int count{ 0 };
	double m{ 0 };
	bool ready{ false };

public:
	int dimension{ 0 };
	void Init(const int size, const double falfa, const double salfa, const int period);
	bool Vector(vector <double>* x, const double bid, const double ask, const int sec);
	void DeInit(void);
};

void CData::Init(const int size, const double falfa, const double salfa, const int period)
{
	_size = size;
	dimension = size * 3;
	_falfa = falfa;
	_salfa = salfa;
	_period = period;

	fma.resize(_size);
	sma.resize(_size);
	mom.resize(_size);

	count = 0;
	last_period = 0;
	m = 2.0 / 86400.0;
}
void CData::DeInit(void)
{
	dimension = 0;
	_size = 0;
	count = 0;
	last_period = 0;
	fma.clear();
	sma.clear();
}
bool CData::Vector(vector <double>* x, const double bid, const double ask, const int sec)
{
	if (sec - last_period >= _period)
	{
		double price = (ask + bid) * 0.5;

		rotate(fma.begin(), fma.begin() + 1, fma.end());
		rotate(sma.begin(), sma.begin() + 1, sma.end());
		rotate(mom.begin(), mom.begin() + 1, mom.end());

		fma[fma.size() - 1] = count == 0 ? price : (1 - _falfa) * price + _falfa * fma[fma.size() - 2];
		sma[sma.size() - 1] = count == 0 ? price : (1 - _salfa) * price + _salfa * sma[sma.size() - 2];
		mom[mom.size() - 1] = -1.0 + (double)sec * m;

		last_period = sec;
		count++;
	}
	else
	{
		return(false);
	}

	if (count < _size)return(false);

	if (!ready)
	{
		ready = true;
		printf("%s: Vector ready!\n", g_cserver_time);
	}

	x->clear();
	x->insert(x->end(), fma.begin(), fma.end());
	x->insert(x->end(), sma.begin(), sma.end());
	NormalizeData(x);

	x->insert(x->end(), mom.begin(), mom.end());
	NormalizeVector(x);

	return(true);
}
void CData::NormalizeVector(vector <double>* x)
{
	double z = 0.0;

	for (size_t i = 0; i < x->size(); i++)
		z += x->at(i) * x->at(i);
	z = sqrt(z);
	for (size_t i = 0; i < x->size(); i++)
		if (z > __DBL_EPSILON__)x->at(i) /= z;
		else x->at(i) = 0;
}
void CData::NormalizeData(vector <double>* x)
{
	double x_min = x->at(distance(x->begin(), min_element(x->begin(), x->end())));
	double x_max = x->at(distance(x->begin(), max_element(x->begin(), x->end())));;
	for (size_t i = 0; i < x->size(); i++)
		if (x_max - x_min != 0)x->at(i) = (x->at(i) - x_min) * 2.0 / (x_max - x_min) - 1.0;
		else x->at(i) = 0;
}
