#include "DualspecModuleConfig.h"

namespace cmangos_module
{
    DualSpecModuleConfig::DualSpecModuleConfig()
    : ModuleConfig("dualspec.conf")
    , enabled(false)
    , cost(0U)
    {
    
    }

    bool DualSpecModuleConfig::OnLoad()
    {
        enabled = config.GetBoolDefault("Dualspec.Enable", false);
        cost = config.GetIntDefault("Dualspec.Cost", 10000U);
        return true;
    }
}