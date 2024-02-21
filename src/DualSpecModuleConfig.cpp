#include "DualSpecModuleConfig.h"
#include "Log.h"

DualSpecModuleConfig::DualSpecModuleConfig()
: ModuleConfig("dualspec.conf")
, enabled(false)
, cost(0U)
{
    
}

bool DualSpecModuleConfig::OnLoad()
{
    sLog.outString("Loading Dual Spec module configuration");

    enabled = config.GetBoolDefault("Dualspec.Enable", false);
    cost = config.GetIntDefault("Dualspec.Cost", 10000U);

    sLog.outString("Dual Spec module configuration loaded");
    return true;
}
