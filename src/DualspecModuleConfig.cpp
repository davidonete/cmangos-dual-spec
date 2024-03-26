#include "DualspecModuleConfig.h"

DualSpecModuleConfig::DualSpecModuleConfig()
: CmangosModuleConfig("dualspec.conf")
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
