#pragma once

#include <string>

struct Config {
    int delay_min_ms = 50;
    int delay_max_ms = 300;
    unsigned int seed = 0;
    bool tiene_seed = false;
    int max_movimientos = 2000;
    bool log_consola = true;
};

bool cargar_config(const std::string& ruta, Config& cfg);