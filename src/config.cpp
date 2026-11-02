#include "config.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>


static std::string recortar(const std::string& s) {
    const char* espacios = " \t\r\n";
    std::size_t ini = s.find_first_not_of(espacios);
    if (ini == std::string::npos) {
        return "";
    }
    std::size_t fin = s.find_last_not_of(espacios);
    return s.substr(ini, fin - ini + 1);
}


static bool a_entero(const std::string& s, long& out) {
    char* fin = nullptr;
    errno = 0;
    long v = std::strtol(s.c_str(), &fin, 10);
    if (errno != 0 || fin == s.c_str() || *fin != '\0') {
        return false;
    }
    out = v;
    return true;
}


static bool asignar(const std::string& clave, const std::string& valor, Config& cfg) {
    long v = 0;
    if (!a_entero(valor, v)) {
        return false;
    }
    if (clave == "delay_min_ms") {
        cfg.delay_min_ms = static_cast<int>(v);
    } else if (clave == "delay_max_ms") {
        cfg.delay_max_ms = static_cast<int>(v);
    } else if (clave == "seed") {
        if (v < 0) {
            return false;
        }
        cfg.seed = static_cast<unsigned int>(v);
        cfg.tiene_seed = true;
    } else if (clave == "max_movimientos") {
        cfg.max_movimientos = static_cast<int>(v);
    } else if (clave == "log_consola") {
        if (v != 0 && v != 1) {
            return false;
        }
        cfg.log_consola = (v == 1);
    } else {
        return false;
    }
    return true;
}


bool cargar_config(const std::string& ruta, Config& cfg) {
    std::ifstream archivo(ruta);
    if (!archivo) {
        std::cerr << "Error: no se pudo abrir " << ruta << "\n";
        return false;
    }

    std::string linea;
    int numero = 0;
    while (std::getline(archivo, linea)) {
        numero++;
        linea = recortar(linea);
        if (linea.empty() || linea[0] == '#') {
            continue;
        }
        std::size_t igual = linea.find('=');
        if (igual == std::string::npos) {
            std::cerr << "Error en " << ruta << ":" << numero << ": falta '='\n";
            return false;
        }
        std::string clave = recortar(linea.substr(0, igual));
        std::string valor = recortar(linea.substr(igual + 1));
        if (!asignar(clave, valor, cfg)) {
            std::cerr << "Error en " << ruta << ":" << numero
                      << ": clave o valor invalido (" << clave << ")\n";
            return false;
        }
    }

    if (cfg.delay_min_ms < 0 || cfg.delay_max_ms < cfg.delay_min_ms) {
        std::cerr << "Error: rango de retardo invalido\n";
        return false;
    }
    if (cfg.max_movimientos <= 0) {
        std::cerr << "Error: max_movimientos debe ser positivo\n";
        return false;
    }
    return true;
}