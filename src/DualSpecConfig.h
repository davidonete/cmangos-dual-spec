#pragma once
#include "ModuleConfig.h"

#define MAX_TALENT_RANK 5
#define MAX_TALENT_SPECS 2

#define DUALSPEC_NPC_ENTRY 100601
#define DUALSPEC_ITEM_ENTRY 17731

#define DUALSPEC_NPC_TEXT 50700
#define DUALSPEC_ITEM_TEXT 50701

class DualSpecModuleConfig : public ModuleConfig
{
public:
    DualSpecModuleConfig();
    bool OnLoad() override;

public:
    bool enabled;
    uint32 cost;
};

