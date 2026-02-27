#ifndef __CVI_REG_STATISTIC__
#define __CVI_REG_STATISTIC__

#include <stdio.h>
#include <string>
#include <vector>
#include "../TLibCommon/CommonDef.h"
using namespace std;

typedef enum
{
    RS_STEP = 0,
    RS_FIX,
    RS_TOTAL
} RegStatMode;

class RegStat
{
private:
    string m_name;
    UInt m_address;
    Int m_val_min;
    Int m_val_max;
    UInt m_step;
    vector<int> m_stat;
    vector<int> m_scale;
    UInt m_total_idx;
    RegStatMode m_mode;

public:
    RegStat(string name, UInt addr, int min, int max, int step);
    RegStat(string name, UInt addr, int *p_scale, int number);
    ~RegStat();

    void AddCount(int val);
    string GetName() { return m_name; };
    Int GetValMin() { return m_val_min; };
    Int GetValMax() { return m_val_max; };
    Int GetStep() { return m_step; };
    vector<Int> GetStat() { return m_stat; };
    int* GetData() { return m_stat.data(); };
    RegStatMode GetMode() { return m_mode; };
    vector<Int> GetScale() { return m_scale; };
};

class CviRegStatistic
{
private:
    vector<RegStat> m_reg_stat;
    bool AddNewItem(string name, UInt addr, int min, int max, int step);
    bool AddNewItem(string name, int *p_scale, int number);
public:
    CviRegStatistic();
    ~CviRegStatistic();

    void InitStatItems();
    RegStat* GetRegStat(string name);
    bool AddItemCount(string name, int val);
    void Export();
    void Import();
    void Report();
};

extern bool g_en_fpga_stat;
extern CviRegStatistic g_fpga_stat;

#endif /* __CVI_REG_STATISTIC__ */
