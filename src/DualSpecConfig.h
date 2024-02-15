#pragma once

#include "Config/Config.h"

#define MAX_TALENT_RANK 5
#define MAX_TALENT_SPECS 2

#define DUALSPEC_NPC_ENTRY 100601
#define DUALSPEC_ITEM_ENTRY 17731

#define DUALSPEC_NPC_TEXT 50700

class DualSpecConfig
{
public:
    DualSpecConfig();

    bool Initialize();

public:
    bool enabled;
    uint32 cost;

private:
    Config config;
};

#define sDualSpecConfig MaNGOS::Singleton<DualSpecConfig>::Instance()

