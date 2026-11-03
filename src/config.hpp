#pragma once

#include <string>
#include <vector>

#include "tablero.hpp"

struct Config {
    int delay_min_ms = 50;
    int delay_max_ms = 300;
    unsigned int seed = 0;
    bool tiene_seed = false;
    int max_movimientos = 2000;
    bool log_consola = true;
    int llegada_max_ms = 200;
    int puma_inicial = NODO_TRONO;
    std::vector<int> perros_iniciales = {13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
    int prob_abandono = 0;
};

bool cargar_config(const std::string& ruta, Config& cfg);
