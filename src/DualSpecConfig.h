#pragma once

#include "Config/Config.h"

#define MAX_TALENT_RANK 5
#define MAX_TALENT_SPECS 2

class DualSpecConfig
{
public:
    DualSpecConfig();

    bool Initialize();

public:
    bool enabled;

private:
    Config config;
};

#define sDualSpecConfig MaNGOS::Singleton<DualSpecConfig>::Instance()

