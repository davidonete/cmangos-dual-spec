#pragma once

#include "Config/Config.h"

class DualspecConfig
{
public:
    DualspecConfig();

    bool Initialize();

public:
    bool enabled;

private:
    Config config;
};

#define sDualspecConfig MaNGOS::Singleton<DualspecConfig>::Instance()

