#pragma once
#include "define.h"
class CFNN
{
private:
    vector <double> weight;
    vector <double> out;
    vector <int> dir;
    vector <int> rating;
    vector <bool> buzy;
    vector <int> id;
    vector <long> born_date;
    vector <long> use_date;
    size_t dimension{ 0 };
    char file_name[CSIZE]{ 0 };
    int max_id{ 0 };
    size_t neurons_total{ 0 };
    bool Load(void);
    void Save(void);

public:
    int max_rating{ 0 };
    void Init(const int _dimension, const char* _file_name, const bool _load);
    void Calc(const vector <double>* _x, SNetOut* _net, const long dt);
    int Training(const vector <double>* _x, const int _dir, const long dt);
    void SetRating(const vector <STradeInfo>* _trade_info);
    void SetBuzy(const int _id, const bool _buzy);
    void Reset(void);
    void Delete(const size_t _index);
    void DeInit(void);
};

void CFNN::Init(const int _dimension, const char* _file_name, const bool _load)
{
    strcpy(file_name, _file_name);
    dimension = _dimension;

    if (_load)Load();
}
void CFNN::DeInit(void)
{
    Save();
    dir.clear();
    weight.clear();
    rating.clear();
    buzy.clear();
    id.clear();
    out.clear();
    born_date.clear();
    use_date.clear();

    neurons_total = 0;
    max_id = 0;
}
void CFNN::SetRating(const vector <STradeInfo>* _trade_info)
{
    for (size_t i = 0; i < _trade_info->size(); i++)
    {
        vector<int>::iterator it = find(id.begin(), id.end(), _trade_info->at(i).id);
        size_t index = distance(id.begin(), it);
        if (index < 0 || id[index] != _trade_info->at(i).id || !buzy[index])
        {
            continue;
        }

        rating[index] += _trade_info->at(i).rating;

        if (rating[index] <= NeuronDelRating)
        {
            Delete(index);
        }
        else
        {
            buzy[index] = false;
            max_rating = rating[max_element(rating.begin(), rating.end()) - rating.begin()];
        }
    }
}
void CFNN::SetBuzy(const int _id, const bool _buzy)
{
    vector<int>::iterator it = find(id.begin(), id.end(), _id);
    size_t index = (int)distance(id.begin(), it);
    if (index < 0 || id[index] != _id)
    {
        return;
    }
    buzy[index] = _buzy;
}
void CFNN::Calc(const vector <double>* _x, SNetOut* _net, const long dt)
{
    _net->buzy = false;
    _net->dir = 0;
    _net->out = 0;
    _net->id = -1;
    _net->rating = 0;
    _net->date = 0;

    if (neurons_total <= 0) return;
    out.assign(neurons_total, 0.0);
    int i = 0;
    int mr = 0;
    int index = -1;

#pragma omp parallel for
    for (i = 0; i < (int)neurons_total; i++)
    {
        size_t _shift = (size_t)i * dimension;

        for (size_t j = 0; j < dimension; j++)
        {
            out[i] += _x->at(j) * weight[_shift + j];
        }

        if (out[i] >= threshold && rating[i] > mr)
        {
            mr = rating[i];
            index = i;
        }
    }

    if (index < 0)index = (int)(max_element(out.begin(), out.end()) - out.begin());

    _net->buzy = buzy[index];
    _net->dir = dir[index];
    _net->id = id[index];
    _net->out = out[index];
    _net->rating = rating[index];
    _net->date = born_date[index];
    use_date[index] = dt;
}
int CFNN::Training(const vector <double>* _x, const int _dir, const long dt)
{
    neurons_total++;
    dir.push_back(_dir);
    rating.push_back(0);
    buzy.push_back(false);
    out.push_back(0.0);
    weight.insert(weight.end(), _x->begin(), _x->end());
    born_date.push_back(dt);
    use_date.push_back(dt);

    max_id++;
    id.push_back(max_id);

    return(id[neurons_total - 1]);
}
void CFNN::Delete(const size_t _index)
{
    neurons_total--;

    weight.erase(weight.begin() + _index * dimension, weight.begin() + (_index + 1) * dimension);
    dir.erase(dir.begin() + _index);
    out.erase(out.begin() + _index);
    rating.erase(rating.begin() + _index);
    id.erase(id.begin() + _index);
    buzy.erase(buzy.begin() + _index);
    born_date.erase(born_date.begin() + _index);
    use_date.erase(use_date.begin() + _index);
    if (neurons_total > 0)
        max_rating = rating[max_element(rating.begin(), rating.end()) - rating.begin()];
}
void CFNN::Save(void)
{
    int net_refer[2]{ (int)neurons_total, (int)dimension };
    FILE* fh = 0;

    fh = fopen(file_name, "wb");
    if (fh)
    {
        fwrite(net_refer, sizeof(int), 2, fh);
        fwrite(weight.data(), sizeof(double), weight.size(), fh);
        fwrite(dir.data(), sizeof(int), dir.size(), fh);
        fwrite(rating.data(), sizeof(int), rating.size(), fh);
        fwrite(id.data(), sizeof(int), id.size(), fh);
        fwrite(born_date.data(), sizeof(long), born_date.size(), fh);
        fwrite(use_date.data(), sizeof(long), use_date.size(), fh);

        fclose(fh);

        printf("%s: Net save done - %s. N: %lu\n", g_cserver_time, file_name, neurons_total);
    }
}
bool CFNN::Load(void)
{
    int net_refer[2]{ 0 };
    FILE* fh = 0;

    fh = fopen(file_name, "rb");
    if (fh)
    {
        fread(net_refer, sizeof(int), 2, fh);
        if ((int)dimension != net_refer[1] || net_refer[0] == 0)
        {
            fclose(fh);
            return(false);
        }

        neurons_total = net_refer[0];

        weight.resize(static_cast<size_t>(neurons_total) * static_cast<size_t>(dimension));
        fread(weight.data(), sizeof(double), weight.size(), fh);

        dir.resize(neurons_total);
        fread(dir.data(), sizeof(int), dir.size(), fh);

        rating.resize(neurons_total);
        fread(rating.data(), sizeof(int), rating.size(), fh);

        id.resize(neurons_total);
        fread(id.data(), sizeof(int), id.size(), fh);

        born_date.resize(neurons_total);
        fread(born_date.data(), sizeof(long), born_date.size(), fh);

        use_date.resize(neurons_total);
        fread(use_date.data(), sizeof(long), use_date.size(), fh);

        fclose(fh);

        if (neurons_total > 0)
        {
            max_id = id[max_element(id.begin(), id.end()) - id.begin()];
            max_rating = rating[max_element(rating.begin(), rating.end()) - rating.begin()];
        }
        
        buzy.assign(neurons_total, false);
        out.assign(neurons_total, 0.0);

        printf("%s: Net load done - %s. N: %lu\n", g_cserver_time, file_name, neurons_total);
    }

    return(true);
}
void CFNN::Reset(void)
{
    buzy.assign(neurons_total, false);

    for (int i = (int)neurons_total - 1; i >= 0; i--)
    {
        if (rating[i] <= NeuronDelRating)Delete(i);
    }
}
