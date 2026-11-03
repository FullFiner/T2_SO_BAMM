#pragma once

#include <pthread.h>

#include <array>

#include "config.hpp"
#include "juego.hpp"

constexpr int NUM_FICHAS = 1 + NUM_PERROS;

constexpr int TABLERO_CADA = 10;

struct Partida {
    Config cfg;
    Estado estado;
    pthread_mutex_t candado;
    pthread_cond_t cambio;

    std::array<int, NUM_FICHAS> pos;
    std::array<bool, NUM_FICHAS> viva;

    bool terminada = false;
    bool interrumpida = false;
    Resultado resultado = Resultado::EN_CURSO;
    int movimientos = 0;
    int id_mesa = 0;

    int lado_caido = -1;
    int mov_caida = 0;
};

void partida_iniciar(Partida& p, const Config& cfg, int id_mesa);

Resultado partida_jugar(Partida& p);

void partida_interrumpir(Partida& p);

void partida_destruir(Partida& p);
