#include "config.hpp"

#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>

#include "juego.hpp"

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

static bool a_lista(const std::string& s, std::vector<int>& out) {
    out.clear();
    std::size_t ini = 0;
    while (true) {
        std::size_t coma = s.find(',', ini);
        std::string trozo = recortar(s.substr(ini, coma == std::string::npos ? std::string::npos : coma - ini));
        long v = 0;
        if (!a_entero(trozo, v) || v < 0 || v >= NUM_NODOS) {
            return false;
        }
        out.push_back(static_cast<int>(v));
        if (coma == std::string::npos) {
            return true;
        }
        ini = coma + 1;
    }
}


static bool asignar(const std::string& clave, const std::string& valor, Config& cfg) {
    if (clave == "perros") {
        return a_lista(valor, cfg.perros_iniciales);
    }
    long v = 0;
    if (!a_entero(valor, v)) {
        return false;
    }

    long tope = (clave == "seed") ? static_cast<long>(UINT_MAX) : static_cast<long>(INT_MAX);
    if (v < 0 || v > tope) {
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
    } else if (clave == "llegada_max_ms") {
        cfg.llegada_max_ms = static_cast<int>(v);
    } else if (clave == "puma") {
        if (v >= NUM_NODOS) {
            return false;
        }
        cfg.puma_inicial = static_cast<int>(v);
    } else if (clave == "prob_abandono") {
        if (v > 100) {
            return false;
        }
        cfg.prob_abandono = static_cast<int>(v);
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
    if (static_cast<int>(cfg.perros_iniciales.size()) != NUM_PERROS) {
        std::cerr << "Error: 'perros' debe listar exactamente " << NUM_PERROS << " nodos\n";
        return false;
    }
    std::set<int> ocupados(cfg.perros_iniciales.begin(), cfg.perros_iniciales.end());
    if (static_cast<int>(ocupados.size()) != NUM_PERROS) {
        std::cerr << "Error: 'perros' tiene nodos repetidos\n";
        return false;
    }
    if (ocupados.count(cfg.puma_inicial) != 0) {
        std::cerr << "Error: el puma y un perro empiezan en el mismo nodo (" << cfg.puma_inicial << ")\n";
        return false;
    }
    if (cfg.llegada_max_ms < 0) {
    std::cerr << "Error: llegada_max_ms no puede ser negativo\n";
    return false;
    }

    return true;
}
