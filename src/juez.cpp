#include "juez.hpp"

#include <string>

#include "partida.hpp"
#include "registro.hpp"


static const char* nombre(Resultado r) {
    switch (r) {
        case Resultado::GANA_PUMA:    return "gana el Puma (captura 6 perros)";
        case Resultado::GANAN_PERROS: return "ganan los Perros (Puma acorralado)";
        case Resultado::EMPATE:       return "empate (maximo de movimientos)";
        default:                      return "en curso";
    }
}


Resultado juez_mesa(int id_mesa, const Config& cfg) {
    std::string mesa = "[mesa " + std::to_string(id_mesa) + "] ";

    Partida p;
    partida_iniciar(p, cfg, id_mesa);
    registro_escribir(mesa + "Comienza la partida\n" + dibujar(p.estado));

    Resultado r = partida_jugar(p);

    registro_escribir(mesa + "Fin: " + nombre(r) + " tras " +
                      std::to_string(p.movimientos) + " movimientos\n" + dibujar(p.estado));
    partida_destruir(p);
    return r;
}