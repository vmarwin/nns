#pragma once
#include "define.h"
class CTrade
{
private:
	int mtpl{ 0 };
	double point_step{ 0 };
	double point_cost{ 0 };
	vector <SPosition> pos;

public:
	int pos_dir{ 0 };
	bool Open(int dir, int id, double bid, double ask, double volume, bool paper, int sec);
	bool Tracking(const double bid, const double ask, const int sec, vector <STradeInfo>* t_info, bool* r_pos);
	bool Close(const double bid, const double ask, vector <STradeInfo>* t_info, bool* r_pos);
	void Init(const int _mtpl, const double p_step, const double p_cost);
	void DeInit(void);
};
void CTrade::Init(const int _mtpl, const double p_step, const double p_cost)
{
	mtpl = _mtpl;
	point_step = p_step;
	point_cost = p_cost;

	pos.clear();
	pos_dir = 0;
}
void CTrade::DeInit(void)
{
	pos.clear();
	pos_dir = 0;
}
bool CTrade::Open(int dir, int id, double bid, double ask, double volume, bool paper, int sec)
{
	pos.push_back(SPosition(id, dir, (dir == buy ? ask : bid), volume, sec, paper));
	if (!paper)pos_dir = dir;

	return(true);
}
bool CTrade::Tracking(const double bid, const double ask, const int sec, vector <STradeInfo>* t_info, bool* r_pos)
{
	t_info->clear();
	*r_pos = false;
	double profit = 0;

	for (int i = (int)pos.size() - 1; i >= 0; i--)
	{
		if (sec - pos[i].open_time > mtpl)
		{
			profit = pos[i].dir == buy ? (bid - pos[i].open_price) / point_step * point_cost * pos[i].volume :
				(pos[i].open_price - ask) / point_step * point_cost * pos[i].volume;

			int r = profit > 0 ? 1 : -1;
			t_info->push_back(STradeInfo(pos[i].id, r));

			if (!pos[i].paper)
			{
				pos_dir = 0;
				*r_pos = true;
			}

			pos.erase(pos.begin() + i);
		}
	}

	return(bool(t_info->size()));
}
bool CTrade::Close(const double bid, const double ask, vector <STradeInfo>* t_info, bool* r_pos)
{
	t_info->clear();
	*r_pos = false;
	double profit = 0;

	for (int i = (int)pos.size() - 1; i >= 0; i--)
	{
		profit = pos[i].dir == buy ? (bid - pos[i].open_price) / point_step * point_cost * pos[i].volume :
			(pos[i].open_price - ask) / point_step * point_cost * pos[i].volume;

		int r = profit > 0 ? 1 : -1;
		t_info->push_back(STradeInfo(pos[i].id, r));

		if (!pos[i].paper)
		{
			pos_dir = 0;
			*r_pos = true;
		}

		pos.erase(pos.begin() + i);
	}

	return(bool(t_info->size()));
}
