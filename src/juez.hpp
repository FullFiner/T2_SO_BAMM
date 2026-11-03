#pragma once

#include "config.hpp"
#include "partida.hpp"

void juez_preparar(Partida& p, const Config& cfg, int id_mesa);

Resultado juez_arbitrar(Partida& p);

void juez_cerrar(Partida& p);