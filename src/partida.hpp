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

    std::array<int, NUM_FICHAS> pos;     
    std::array<bool, NUM_FICHAS> viva;   

    bool terminada = false;
    Resultado resultado = Resultado::EN_CURSO;
    int movimientos = 0;
    int id_mesa = 0;
};

void partida_iniciar(Partida& p, const Config& cfg, int id_mesa);


Resultado partida_jugar(Partida& p);

void partida_destruir(Partida& p);