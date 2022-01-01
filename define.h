#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <cgate.h>
#include <omp.h>

using namespace std;

constexpr int buy = 1;
constexpr int sell = 2;
constexpr int ret = 1;
constexpr int iok = 2;
constexpr int fok = 3;
constexpr int CSIZE = 80;
constexpr double threshold = 0.99;
constexpr int NeuronDelRating = 0;

struct AddOrder
{
	char broker_code[5];            // c4
	signed int isin_id;             // i4
	char client_code[4];            // c3
	signed int dir;                 // i4
	signed int type;                // i4
	signed int amount;              // i4
	char price[18];                 // c17
	char comment[21];               // c20
	char broker_to[21];             // c20
	signed int ext_id;              // i4
	signed int is_check_limit;      // i4
	char date_exp[9];               // c8
	signed int dont_check_money;    // i4
	char match_ref[11];             //c10
	signed int ncc_request;         //i1
};
struct CODHeartbeat
{
	signed int seq_number;             // i4
};
struct SPosition
{
	int id;
	int dir;
	double open_price;
	double volume;
	int open_time;
	bool paper;

	SPosition(int& _id, int& _dir, double& _open_price, double& _volume, int& _open_time, bool& _paper) :
		id(_id), dir(_dir), open_price(_open_price), volume(_volume), open_time(_open_time), paper(_paper) {};
};
struct SNetOut
{
	double out{ 0 };
	int dir{ 0 };
	int rating{ 0 };
	bool buzy{ false };
	int id{ 0 };
	long date{ 0 };
};
struct STradeInfo
{
	int id{ 0 };
	int rating{ 0 };

	STradeInfo(int& _id, int& _rating) : id(_id), rating(_rating) {};
};
char g_cserver_time[CSIZE];