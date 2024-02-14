#pragma once

#include "Config/Config.h"

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

