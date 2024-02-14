#ifndef MANGOS_DUALSPEC_MGR_H
#define MANGOS_DUALSPEC_MGR_H

class DualSpecMgr
{
public:
    DualSpecMgr() {}

    void Init();
};

#define sDualSpecMgr MaNGOS::Singleton<DualSpecMgr>::Instance()
#endif