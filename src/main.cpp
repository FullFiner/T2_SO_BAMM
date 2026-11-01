#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>


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

    std::string config = (argc == 4) ? argv[3] : "";

    std::cout << "Jugadores: " << n << ", mesas: " << m;
    if (!config.empty()) {
        std::cout << ", config: " << config;
    }
    std::cout << "\n";

    return 0;
}