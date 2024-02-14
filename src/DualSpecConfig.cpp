#include "DualspecConfig.h"

#include "Globals/ObjectMgr.h"
#include "Log.h"
#include "SystemConfig.h"

DualspecConfig::DualspecConfig()
: enabled(false)
{
    
}

INSTANTIATE_SINGLETON_1(DualspecConfig);

bool DualspecConfig::Initialize()
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
