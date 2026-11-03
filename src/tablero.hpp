#pragma once

#include <vector>

constexpr int NUM_NODOS = 28;
constexpr int NODO_INTERMEDIO_IZQ = 25;
constexpr int NODO_INTERMEDIO_DER = 26;
constexpr int NODO_TRONO = 27;

struct Salto {
    int sobre;
    int destino;
};

struct Topologia {
    std::vector<int> vecinos[NUM_NODOS];
    std::vector<Salto> saltos[NUM_NODOS];
};

const Topologia& topologia();
