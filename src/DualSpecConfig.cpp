#include "DualSpecConfig.h"

#include "Log.h"
#include "SystemConfig.h"

DualSpecConfig::DualSpecConfig()
: enabled(false)
, cost(0U)
{
    
}

INSTANTIATE_SINGLETON_1(DualSpecConfig);

bool DualSpecConfig::Initialize()
{
    sLog.outString("Initializing Dual Spec");

    if (!config.SetSource(SYSCONFDIR"dualspec.conf"))
    {
        sLog.outError("Failed to open configuration file dualspec.conf");
        return false;
    }

    enabled = config.GetBoolDefault("Dualspec.Enable", false);
    cost = config.GetIntDefault("Dualspec.Cost", 10000U);

    sLog.outString("Dual Spec configuration loaded");
    return true;
}
