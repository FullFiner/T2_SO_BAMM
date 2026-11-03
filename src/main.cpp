#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>

#include "config.hpp"
#include "registro.hpp"
#include "torneo.hpp"

constexpr int MAX_JUGADORES = 10000;
constexpr int MAX_MESAS = 1000;

static void uso(const char* prog) {
    std::cerr << "Uso: " << prog << " N M [config.txt]\n"
              << "  N: numero total de jugadores (1 a " << MAX_JUGADORES << ")\n"
              << "  M: maximo de mesas concurrentes (1 a " << MAX_MESAS << ")\n";
}

static bool parsear_entero(const char* s, int maximo, int& out) {
    char* fin = nullptr;
    errno = 0;
    long v = std::strtol(s, &fin, 10);
    if (errno != 0 || fin == s || *fin != '\0' || v <= 0 || v > maximo) {
        return false;
    }
    out = static_cast<int>(v);
    return true;
}

int main(int argc, char** argv) {

    std::signal(SIGPIPE, SIG_IGN);

    if (argc < 3 || argc > 4) {
        uso(argv[0]);
        return 1;
    }

    int n = 0;
    int m = 0;
    if (!parsear_entero(argv[1], MAX_JUGADORES, n) || !parsear_entero(argv[2], MAX_MESAS, m)) {
        std::cerr << "Error: N y M deben ser enteros dentro del rango permitido\n";
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

    if (!registro_abrir("torneo.log", cfg.log_consola)) {
        std::cerr << "Error: no se pudo abrir torneo.log\n";
        return 1;
    }

    torneo_ejecutar(n, m, cfg);

    registro_cerrar();
    return 0;
}
