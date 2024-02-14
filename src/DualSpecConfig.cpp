#include "DualSpecConfig.h"

#include "Globals/ObjectMgr.h"
#include "Log.h"
#include "SystemConfig.h"

DualSpecConfig::DualSpecConfig()
: enabled(false)
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

    sLog.outString("Dual Spec configuration loaded");
    return true;
}
