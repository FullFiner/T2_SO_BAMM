#include "tablero.hpp"

#include <algorithm>
#include <cstddef>


static std::vector<std::vector<int>> lineas() {
    std::vector<std::vector<int>> l;

    for (int f = 0; f < 5; f++) {
        std::vector<int> fila;
        for (int c = 0; c < 5; c++) {
            fila.push_back(f * 5 + c);
        }
        l.push_back(fila);
    }

    for (int c = 0; c < 5; c++) {
        std::vector<int> col;
        if (c == 1) {
            col.push_back(NODO_INTERMEDIO_IZQ);
        } else if (c == 2) {
            col.push_back(NODO_TRONO);
        } else if (c == 3) {
            col.push_back(NODO_INTERMEDIO_DER);
        }
        for (int f = 0; f < 5; f++) {
            col.push_back(f * 5 + c);
        }
        l.push_back(col);
    }

    l.push_back({NODO_INTERMEDIO_IZQ, NODO_INTERMEDIO_DER});
    l.push_back({NODO_TRONO, NODO_INTERMEDIO_IZQ, 0});
    l.push_back({NODO_TRONO, NODO_INTERMEDIO_DER, 4});
    return l;
}


static void agregar_vecino(Topologia& t, int a, int b) {
    if (std::find(t.vecinos[a].begin(), t.vecinos[a].end(), b) == t.vecinos[a].end()) {
        t.vecinos[a].push_back(b);
    }
    if (std::find(t.vecinos[b].begin(), t.vecinos[b].end(), a) == t.vecinos[b].end()) {
        t.vecinos[b].push_back(a);
    }
}


static Topologia construir() {
    Topologia t;
    for (const auto& linea : lineas()) {
        for (std::size_t i = 0; i + 1 < linea.size(); i++) {
            agregar_vecino(t, linea[i], linea[i + 1]);
        }
        for (std::size_t i = 0; i + 2 < linea.size(); i++) {
            int a = linea[i];
            int b = linea[i + 1];
            int c = linea[i + 2];
            t.saltos[a].push_back({b, c});
            t.saltos[c].push_back({b, a});
        }
    }
    return t;
}


const Topologia& topologia() {
    static const Topologia t = construir();
    return t;
}