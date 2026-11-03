#include "partida.hpp"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include "registro.hpp"

struct ArgsFicha {
    Partida* partida;
    int id;
};

static int azar(std::mt19937& rng, int minimo, int maximo) {
    return std::uniform_int_distribution<int>(minimo, maximo)(rng);
}

static void registrar(Partida& p, const std::string& texto) {
    p.movimientos++;
    std::string linea = "[mesa " + std::to_string(p.id_mesa) + " | mov " +
                        std::to_string(p.movimientos) + "] " + texto + "\n";
    if (p.movimientos % TABLERO_CADA == 0) {
        linea += "[mesa " + std::to_string(p.id_mesa) + " | tablero tras mov " +
                 std::to_string(p.movimientos) + "]\n" + dibujar(p.estado);
    }
    registro_escribir(linea);
}

static void terminar(Partida& p, Resultado r) {
    if (!p.terminada) {
        p.terminada = true;
        p.resultado = r;
        pthread_cond_broadcast(&p.cambio);
    }
}

static void esperar_turno(Partida& p, int ms) {
    timespec limite;
    clock_gettime(CLOCK_MONOTONIC, &limite);
    limite.tv_sec += ms / 1000;
    limite.tv_nsec += static_cast<long>(ms % 1000) * 1000000L;
    if (limite.tv_nsec >= 1000000000L) {
        limite.tv_sec += 1;
        limite.tv_nsec -= 1000000000L;
    }
    while (!p.terminada) {
        if (pthread_cond_timedwait(&p.cambio, &p.candado, &limite) == ETIMEDOUT) {
            break;
        }
    }
}

static void turno_perro(Partida& p, int id, std::mt19937& rng) {
    int origen = p.pos[id];
    std::vector<int> libres;
    for (int v : topologia().vecinos[origen]) {
        if (p.estado.celdas[v] == Celda::VACIA) {
            libres.push_back(v);
        }
    }
    if (libres.empty()) {
        return;
    }
    int destino = libres[azar(rng, 0, static_cast<int>(libres.size()) - 1)];
    if (mover(p.estado, origen, destino)) {
        p.pos[id] = destino;
        registrar(p, "perro " + std::to_string(id) + ": " +
                         std::to_string(origen) + " -> " + std::to_string(destino));
    }
}

static void turno_puma(Partida& p, std::mt19937& rng) {
    const Topologia& t = topologia();
    int origen = p.estado.pos_puma;

    std::vector<Salto> capturas;
    for (const Salto& s : t.saltos[origen]) {
        if (p.estado.celdas[s.sobre] == Celda::PERRO &&
            p.estado.celdas[s.destino] == Celda::VACIA) {
            capturas.push_back(s);
        }
    }
    if (!capturas.empty()) {
        Salto s = capturas[azar(rng, 0, static_cast<int>(capturas.size()) - 1)];
        if (capturar(p.estado, origen, s.sobre, s.destino)) {
            for (int i = 1; i < NUM_FICHAS; i++) {
                if (p.viva[i] && p.pos[i] == s.sobre) {
                    p.viva[i] = false;
                    break;
                }
            }
            registrar(p, "PUMA captura el perro de " + std::to_string(s.sobre) +
                             " (" + std::to_string(origen) + " -> " + std::to_string(s.destino) + ")");
        }
        return;
    }

    std::vector<int> libres;
    for (int v : t.vecinos[origen]) {
        if (p.estado.celdas[v] == Celda::VACIA) {
            libres.push_back(v);
        }
    }
    if (libres.empty()) {
        return;
    }
    int destino = libres[azar(rng, 0, static_cast<int>(libres.size()) - 1)];
    if (mover(p.estado, origen, destino)) {
        registrar(p, "PUMA: " + std::to_string(origen) + " -> " + std::to_string(destino));
    }
}

static void ficha_cayo(void* arg) {
    ArgsFicha* a = static_cast<ArgsFicha*>(arg);
    Partida& p = *a->partida;
    bool es_puma = (a->id == 0);

    registro_escribir("[mesa " + std::to_string(p.id_mesa) + "] el jugador " +
                      (es_puma ? "del Puma" : "de los Perros") + " se cae (la ficha " +
                      std::to_string(a->id) + " murio con el candado tomado)\n");
    terminar(p, es_puma ? Resultado::PUMA_ABANDONA : Resultado::PERROS_ABANDONAN);
    pthread_mutex_unlock(&p.candado);
}

static void* hilo_ficha(void* arg) {
    ArgsFicha* a = static_cast<ArgsFicha*>(arg);
    Partida& p = *a->partida;
    int id = a->id;
    int lado = (id == 0) ? 0 : 1;
    std::mt19937 rng(p.cfg.seed + 1000u * static_cast<unsigned>(p.id_mesa) + static_cast<unsigned>(id));

    pthread_cleanup_push(ficha_cayo, a);
    pthread_mutex_lock(&p.candado);
    while (true) {
        int espera = azar(rng, p.cfg.delay_min_ms, p.cfg.delay_max_ms);
        esperar_turno(p, espera);

        if (p.terminada || (id != 0 && !p.viva[id])) {
            break;
        }

        if (p.lado_caido == lado && p.movimientos >= p.mov_caida) {
            pthread_exit(nullptr);
        }

        if (id == 0) {
            turno_puma(p, rng);
        } else {
            turno_perro(p, id, rng);
        }

        Resultado r = evaluar(p.estado);
        if (r != Resultado::EN_CURSO) {
            terminar(p, r);
        } else if (p.movimientos >= p.cfg.max_movimientos) {
            terminar(p, Resultado::EMPATE);
        }
    }
    pthread_mutex_unlock(&p.candado);
    pthread_cleanup_pop(0);
    return nullptr;
}

void partida_iniciar(Partida& p, const Config& cfg, int id_mesa) {
    p.cfg = cfg;
    p.id_mesa = id_mesa;
    p.terminada = false;
    p.interrumpida = false;
    p.resultado = Resultado::EN_CURSO;
    p.movimientos = 0;
    inicializar(p.estado);

    p.lado_caido = -1;
    p.mov_caida = 0;
    int tope = std::min(40, cfg.max_movimientos - 1);
    if (cfg.prob_abandono > 0 && tope >= 1) {
        std::mt19937 rng_caida(cfg.seed + 5003u * static_cast<unsigned>(id_mesa) + 17u);
        if (azar(rng_caida, 1, 100) <= cfg.prob_abandono) {
            p.lado_caido = azar(rng_caida, 0, 1);
            p.mov_caida = azar(rng_caida, 1, tope);
        }
    }

    p.pos.fill(-1);
    p.viva.fill(false);
    int nodo = 13;
    for (int i = 1; i < NUM_FICHAS; i++) {
        p.pos[i] = nodo++;
        p.viva[i] = true;
    }
    pthread_mutex_init(&p.candado, nullptr);
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&p.cambio, &attr);
    pthread_condattr_destroy(&attr);
}

Resultado partida_jugar(Partida& p) {
    pthread_t hilos[NUM_FICHAS];
    ArgsFicha args[NUM_FICHAS];
    int creados = 0;

    for (int i = 0; i < NUM_FICHAS; i++) {
        args[i] = {&p, i};
        if (pthread_create(&hilos[i], nullptr, hilo_ficha, &args[i]) != 0) {
            std::cerr << "Error: no se pudo crear el hilo de la ficha " << i << "\n";
            pthread_mutex_lock(&p.candado);
            terminar(p, Resultado::EMPATE);
            pthread_mutex_unlock(&p.candado);
            break;
        }
        creados++;
    }

    pthread_mutex_lock(&p.candado);
    while (!p.terminada && !p.interrumpida) {
        pthread_cond_wait(&p.cambio, &p.candado);
    }
    if (!p.terminada) {
        terminar(p, Resultado::INTERRUMPIDA);
    }
    pthread_mutex_unlock(&p.candado);

    for (int i = 0; i < creados; i++) {
        pthread_join(hilos[i], nullptr);
    }

    return p.resultado;
}

void partida_interrumpir(Partida& p) {
    pthread_mutex_lock(&p.candado);
    if (!p.terminada) {
        p.interrumpida = true;
        pthread_cond_broadcast(&p.cambio);
    }
    pthread_mutex_unlock(&p.candado);
}

void partida_destruir(Partida& p) {
    pthread_cond_destroy(&p.cambio);

    if (pthread_mutex_destroy(&p.candado) != 0) {
        std::cerr << "Error: la mesa " << p.id_mesa
                  << " no pudo destruir su candado porque seguia tomado\n";
    }
}
