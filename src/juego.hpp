#pragma once

#include <array>
#include <string>

#include "tablero.hpp"

constexpr int NUM_PERROS = 12;
constexpr int CAPTURAS_PARA_GANAR = 6;

enum class Celda { VACIA, PUMA, PERRO };

enum class Resultado { EN_CURSO, GANA_PUMA, GANAN_PERROS, EMPATE };


struct Estado {
    std::array<Celda, NUM_NODOS> celdas;
    int pos_puma = NODO_TRONO;
    int perros_vivos = NUM_PERROS;
    int capturas = 0;
};


void inicializar(Estado& e);


bool mover(Estado& e, int origen, int destino);

bool capturar(Estado& e, int origen, int sobre, int destino);


bool puma_acorralado(const Estado& e);

Resultado evaluar(const Estado& e);


std::string dibujar(const Estado& e);