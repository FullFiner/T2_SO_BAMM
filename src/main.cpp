#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>
#include <random>
#include "config.hpp"
#include "tablero.hpp"
#include "partida.hpp"


static void uso(const char* prog) {
    std::cerr << "Uso: " << prog << " N M [config.txt]\n"
              << "  N: numero total de jugadores\n"
              << "  M: maximo de mesas concurrentes\n";
}


static bool parsear_entero(const char* s, int& out) {
    char* fin = nullptr;
    errno = 0;
    long v = std::strtol(s, &fin, 10);
    if (errno != 0 || fin == s || *fin != '\0' || v <= 0 || v > 100000) {
        return false;
    }
    out = static_cast<int>(v);
    return true;
}


int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        uso(argv[0]);
        return 1;
    }

    int n = 0;
    int m = 0;
    if (!parsear_entero(argv[1], n) || !parsear_entero(argv[2], m)) {
        std::cerr << "Error: N y M deben ser enteros positivos\n";
        uso(argv[0]);
        return 1;
    }

    std::string ruta = (argc == 4) ? argv[3] : "";

    Config cfg;
    if (!ruta.empty() && !cargar_config(ruta, cfg)) {
        return 1;
    }
    if (!cfg.tiene_seed) {
        cfg.seed = std::random_device{}();
    }

    std::cout << "Jugadores: " << n << ", mesas: " << m << "\n";
    std::cout << "Retardo: " << cfg.delay_min_ms << "-" << cfg.delay_max_ms << " ms\n";
    std::cout << "Seed: " << cfg.seed << "\n";
    std::cout << "Max movimientos: " << cfg.max_movimientos << "\n";

    const Topologia& topo = topologia();
    int aristas = 0;
    int saltos = 0;
    for (int i = 0; i < NUM_NODOS; i++) {
        aristas += static_cast<int>(topo.vecinos[i].size());
        saltos += static_cast<int>(topo.saltos[i].size());
    }
    std::cout << "Nodos: " << NUM_NODOS << ", aristas: " << aristas / 2
              << ", saltos: " << saltos << "\n";


    Partida partida;
    partida_iniciar(partida, cfg, 0);
    partida_jugar(partida);
    partida_destruir(partida);

    
    return 0;
}