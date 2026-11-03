#include "juez.hpp"

#include <string>

#include "registro.hpp"

static const char* nombre(Resultado r) {
    switch (r) {
        case Resultado::GANA_PUMA:    return "gana el Puma (captura 6 perros)";
        case Resultado::GANAN_PERROS: return "ganan los Perros (Puma acorralado)";
        case Resultado::EMPATE:       return "empate (maximo de movimientos)";
        case Resultado::INTERRUMPIDA: return "partida interrumpida por el servidor";
        case Resultado::PUMA_ABANDONA:    return "el Puma abandona: ganan los Perros por abandono";
        case Resultado::PERROS_ABANDONAN: return "los Perros abandonan: gana el Puma por abandono";
        default:                     return "en curso";
    }
}

void juez_preparar(Partida& p, const Config& cfg, int id_mesa) {
    partida_iniciar(p, cfg, id_mesa);
}

Resultado juez_arbitrar(Partida& p) {
    std::string mesa = "[mesa " + std::to_string(p.id_mesa) + "] ";
    registro_escribir(mesa + "Comienza la partida\n" + dibujar(p.estado));

    Resultado r = partida_jugar(p);

    if (r == Resultado::PUMA_ABANDONA || r == Resultado::PERROS_ABANDONAN) {
        registro_escribir(mesa + "El juez detecta el abandono, cierra la partida y libera la mesa\n");
    }

    registro_escribir(mesa + "Fin: " + nombre(r) + " tras " +
                      std::to_string(p.movimientos) + " movimientos\n" + dibujar(p.estado));
    return r;
}

void juez_cerrar(Partida& p) {
    partida_destruir(p);
}
