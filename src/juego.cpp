#include "juego.hpp"

#include <algorithm>
#include <sstream>

static bool nodo_valido(int n) {
    return n >= 0 && n < NUM_NODOS;
}

static bool son_vecinos(int a, int b) {
    const std::vector<int>& v = topologia().vecinos[a];
    return std::find(v.begin(), v.end(), b) != v.end();
}

static bool salto_permitido(int origen, int sobre, int destino) {
    for (const Salto& s : topologia().saltos[origen]) {
        if (s.sobre == sobre && s.destino == destino) {
            return true;
        }
    }
    return false;
}

void inicializar(Estado& e, int puma, const std::vector<int>& perros) {
    e.celdas.fill(Celda::VACIA);
    for (int n : perros) {
        e.celdas[n] = Celda::PERRO;
    }
    e.pos_puma = puma;
    e.celdas[e.pos_puma] = Celda::PUMA;
    e.perros_vivos = NUM_PERROS;
    e.capturas = 0;
}

bool mover(Estado& e, int origen, int destino) {
    if (!nodo_valido(origen) || !nodo_valido(destino)) {
        return false;
    }
    if (e.celdas[origen] == Celda::VACIA || e.celdas[destino] != Celda::VACIA) {
        return false;
    }
    if (!son_vecinos(origen, destino)) {
        return false;
    }
    if (e.celdas[origen] == Celda::PUMA) {
        e.pos_puma = destino;
    }
    e.celdas[destino] = e.celdas[origen];
    e.celdas[origen] = Celda::VACIA;
    return true;
}

bool capturar(Estado& e, int origen, int sobre, int destino) {
    if (!nodo_valido(origen) || !nodo_valido(sobre) || !nodo_valido(destino)) {
        return false;
    }
    if (e.celdas[origen] != Celda::PUMA) {
        return false;
    }
    if (e.celdas[sobre] != Celda::PERRO || e.celdas[destino] != Celda::VACIA) {
        return false;
    }
    if (!salto_permitido(origen, sobre, destino)) {
        return false;
    }
    e.celdas[sobre] = Celda::VACIA;
    e.celdas[origen] = Celda::VACIA;
    e.celdas[destino] = Celda::PUMA;
    e.pos_puma = destino;
    e.perros_vivos--;
    e.capturas++;
    return true;
}

bool puma_acorralado(const Estado& e) {
    const Topologia& t = topologia();
    int p = e.pos_puma;
    for (int v : t.vecinos[p]) {
        if (e.celdas[v] == Celda::VACIA) {
            return false;
        }
    }
    for (const Salto& s : t.saltos[p]) {
        if (e.celdas[s.sobre] == Celda::PERRO && e.celdas[s.destino] == Celda::VACIA) {
            return false;
        }
    }
    return true;
}

Resultado evaluar(const Estado& e) {
    if (e.capturas >= CAPTURAS_PARA_GANAR) {
        return Resultado::GANA_PUMA;
    }
    if (puma_acorralado(e)) {
        return Resultado::GANAN_PERROS;
    }
    return Resultado::EN_CURSO;
}

static char simbolo(Celda c) {
    switch (c) {
        case Celda::PUMA:  return 'P';
        case Celda::PERRO: return 'o';
        default:           return '.';
    }
}

std::string dibujar(const Estado& e) {
    auto s = [&e](int n) { return simbolo(e.celdas[n]); };
    std::ostringstream os;

    os << "          " << s(NODO_TRONO) << "\n";
    os << "      " << s(NODO_INTERMEDIO_IZQ) << "       " << s(NODO_INTERMEDIO_DER) << "\n";
    for (int f = 0; f < 5; f++) {
        os << "  ";
        for (int c = 0; c < 5; c++) {
            os << s(f * 5 + c);
            if (c < 4) {
                os << " - ";
            }
        }
        os << "\n";
        if (f < 4) {
            os << "  |   |   |   |   |\n";
        }
    }
    os << "Perros vivos: " << e.perros_vivos << "  Capturas: " << e.capturas << "\n";
    return os.str();
}
