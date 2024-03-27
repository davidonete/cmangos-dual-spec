#pragma once
#include "ModuleConfig.h"

namespace cmangos_module
{
    #define MAX_TALENT_RANK 5
    #define MAX_TALENT_SPECS 2

    #define DUALSPEC_NPC_ENTRY 100601
    #define DUALSPEC_ITEM_ENTRY 17731

    #define DUALSPEC_NPC_TEXT 50700
    #define DUALSPEC_ITEM_TEXT 50701

    enum DualSpecMessages
    {
        DUAL_SPEC_DESCRIPTION = 12000,
        DUAL_SPEC_COST_IS,
        DUAL_SPEC_CHANGE_MY_SPEC,
        DUAL_SPEC_NO_GOLD_UNLOCK,
        DUAL_SPEC_ARE_YOU_SURE_BEGIN,
        DUAL_SPEC_ARE_YOU_SURE_END,
        DUAL_SPEC_ALREADY_ON_SPEC,
        DUAL_SPEC_ACTIVATE,
        DUAL_SPEC_RENAME,
        DUAL_SPEC_UNNAMED,
        DUAL_SPEC_ACTIVE,
        DUAL_SPEC_ERR_COMBAT,
        DUAL_SPEC_ERR_INSTANCE,
        DUAL_SPEC_ERR_MOUNT,
        DUAL_SPEC_ERR_DEAD,
        DUAL_SPEC_ERR_UNLOCK,
        DUAL_SPEC_ERR_LEVEL,
        DUAL_SPEC_ACTIVATE_COLOR,
        DUAL_SPEC_RENAME_COLOR,
        DUAL_SPEC_ARE_YOU_SURE_SWITCH,
        DUAL_SPEC_PURCHASE,
        DUAL_SPEC_ERR_ITEM_CREATE,
    };

    class DualSpecModuleConfig : public ModuleConfig
    {
    public:
        DualSpecModuleConfig();
        bool OnLoad() override;

    public:
        bool enabled;
        uint32 cost;
    };
}