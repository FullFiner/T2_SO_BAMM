#include "torneo.hpp"

#include <pthread.h>
#include <semaphore.h>

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <ctime>
#include <deque>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "juez.hpp"
#include "registro.hpp"
#include "senales.hpp"

struct Servidor;

struct Jugador {
    int id = 0;
    Servidor* servidor = nullptr;
    pthread_mutex_t candado;
    pthread_cond_t fin;
    bool terminado = false;
};

struct Servidor {
    Config cfg;
    int n = 0;
    int llegados = 0;
    int activas = 0;
    int pico = 0;
    int jugadas = 0;
    int abandonos = 0;
    bool apagado = false;
    std::deque<int> cola;
    std::vector<Partida*> en_juego;
    pthread_mutex_t candado;
    pthread_cond_t llegada;
    pthread_cond_t apagar;
    sem_t mesas;
};

struct Mesa {
    Servidor* servidor = nullptr;
    int id_mesa = 0;
    Jugador* puma = nullptr;
    Jugador* perros = nullptr;
    Partida partida;
};

static void liberar_jugador(Jugador* j) {
    pthread_mutex_lock(&j->candado);
    j->terminado = true;
    pthread_cond_broadcast(&j->fin);
    pthread_mutex_unlock(&j->candado);
}

static void interrumpir(void* ctx) {
    Servidor& s = *static_cast<Servidor*>(ctx);
    registro_escribir("Senal de interrupcion recibida: se detienen las llegadas y se cierran las partidas activas\n");

    pthread_mutex_lock(&s.candado);
    s.apagado = true;
    for (Partida* p : s.en_juego) {
        partida_interrumpir(*p);
    }
    pthread_cond_broadcast(&s.llegada);
    pthread_cond_broadcast(&s.apagar);
    pthread_mutex_unlock(&s.candado);
    sem_post(&s.mesas);
}

static void* hilo_cliente(void* arg) {
    Jugador* j = static_cast<Jugador*>(arg);
    Servidor& s = *j->servidor;
    std::mt19937 rng(s.cfg.seed + 7919u * static_cast<unsigned>(j->id));
    int espera = std::uniform_int_distribution<int>(0, s.cfg.llegada_max_ms)(rng);

    timespec limite;
    clock_gettime(CLOCK_MONOTONIC, &limite);
    limite.tv_sec += espera / 1000;
    limite.tv_nsec += static_cast<long>(espera % 1000) * 1000000L;
    if (limite.tv_nsec >= 1000000000L) {
        limite.tv_sec += 1;
        limite.tv_nsec -= 1000000000L;
    }

    pthread_mutex_lock(&s.candado);
    while (!s.apagado) {
        if (pthread_cond_timedwait(&s.apagar, &s.candado, &limite) == ETIMEDOUT) {
            break;
        }
    }
    if (s.apagado) {
        pthread_mutex_unlock(&s.candado);
        registro_escribir("Jugador " + std::to_string(j->id) + " no entra: el torneo esta cerrando\n");
        return nullptr;
    }
    registro_escribir("Jugador " + std::to_string(j->id) + " llega al torneo\n");
    s.cola.push_back(j->id);

    s.llegados++;
    pthread_cond_signal(&s.llegada);
    pthread_mutex_unlock(&s.candado);

    pthread_mutex_lock(&j->candado);
    while (!j->terminado) {
        pthread_cond_wait(&j->fin, &j->candado);
    }
    pthread_mutex_unlock(&j->candado);

    registro_escribir("Jugador " + std::to_string(j->id) + " termina su participacion\n");
    return nullptr;
}

static void* hilo_mesa(void* arg) {
    Mesa* m = static_cast<Mesa*>(arg);
    Servidor& s = *m->servidor;

    juez_preparar(m->partida, s.cfg, m->id_mesa);

    pthread_mutex_lock(&s.candado);
    s.en_juego.push_back(&m->partida);
    s.activas++;
    if (s.activas > s.pico) {
        s.pico = s.activas;
    }
    if (s.apagado) {
        partida_interrumpir(m->partida);
    }
    pthread_mutex_unlock(&s.candado);

    registro_escribir("[mesa " + std::to_string(m->id_mesa) + "] jugador " +
                      std::to_string(m->puma->id) + " (Puma) vs jugador " +
                      std::to_string(m->perros->id) + " (Perros)\n");
    Resultado r = juez_arbitrar(m->partida);

    bool abandono = (r == Resultado::PUMA_ABANDONA || r == Resultado::PERROS_ABANDONAN);
    if (abandono) {
        bool cayo_puma = (r == Resultado::PUMA_ABANDONA);
        Jugador* cae = cayo_puma ? m->puma : m->perros;
        Jugador* gana = cayo_puma ? m->perros : m->puma;
        registro_escribir("[mesa " + std::to_string(m->id_mesa) + "] jugador " +
                          std::to_string(cae->id) + (cayo_puma ? " (Puma)" : " (Perros)") +
                          " abandona: gana por abandono el jugador " + std::to_string(gana->id) +
                          (cayo_puma ? " (Perros)" : " (Puma)") + "\n");
    }

    pthread_mutex_lock(&s.candado);
    s.en_juego.erase(std::find(s.en_juego.begin(), s.en_juego.end(), &m->partida));
    s.activas--;
    s.jugadas++;
    if (abandono) {
        s.abandonos++;
    }
    pthread_mutex_unlock(&s.candado);

    juez_cerrar(m->partida);
    liberar_jugador(m->puma);
    liberar_jugador(m->perros);
    sem_post(&s.mesas);
    return nullptr;
}

void torneo_ejecutar(int n, int m, const Config& cfg) {
    Servidor s;
    s.cfg = cfg;
    s.n = n;
    pthread_mutex_init(&s.candado, nullptr);
    pthread_cond_init(&s.llegada, nullptr);
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&s.apagar, &attr);
    pthread_condattr_destroy(&attr);
    sem_init(&s.mesas, 0, static_cast<unsigned int>(m));

    if (!senales_iniciar(interrumpir, &s)) {
        registro_escribir("Error: no se pudo iniciar el manejo de senales\n");
        sem_destroy(&s.mesas);
        pthread_cond_destroy(&s.apagar);
        pthread_cond_destroy(&s.llegada);
        pthread_mutex_destroy(&s.candado);
        return;
    }

    std::vector<Jugador> jugadores(n);
    for (int i = 0; i < n; i++) {
        jugadores[i].id = i + 1;
        jugadores[i].servidor = &s;
        pthread_mutex_init(&jugadores[i].candado, nullptr);
        pthread_cond_init(&jugadores[i].fin, nullptr);
    }

    std::vector<pthread_t> clientes;
    for (int i = 0; i < n; i++) {
        pthread_t h;
        if (pthread_create(&h, nullptr, hilo_cliente, &jugadores[i]) != 0) {
            registro_escribir("Error: no se pudo crear el hilo del jugador " +
                              std::to_string(i + 1) + "\n");
            pthread_mutex_lock(&s.candado);
            s.llegados++;
            pthread_cond_signal(&s.llegada);
            pthread_mutex_unlock(&s.candado);
            continue;
        }
        clientes.push_back(h);
    }

    std::deque<Mesa> datos_mesas;
    std::vector<pthread_t> mesas;
    int siguiente = 1;

    while (true) {
        pthread_mutex_lock(&s.candado);
        while (s.cola.size() < 2 && s.llegados < s.n && !s.apagado) {
            pthread_cond_wait(&s.llegada, &s.candado);
        }
        if (s.apagado) {
            pthread_mutex_unlock(&s.candado);
            break;
        }
        if (s.cola.size() < 2) {
            int sobrante = s.cola.empty() ? 0 : s.cola.front();
            pthread_mutex_unlock(&s.candado);
            if (sobrante != 0) {
                registro_escribir("Jugador " + std::to_string(sobrante) + " se queda sin pareja\n");
                liberar_jugador(&jugadores[sobrante - 1]);
            }
            break;
        }
        int a = s.cola.front();
        s.cola.pop_front();
        int b = s.cola.front();
        s.cola.pop_front();
        pthread_mutex_unlock(&s.candado);

        sem_wait(&s.mesas);

        pthread_mutex_lock(&s.candado);
        bool cerrado = s.apagado;
        pthread_mutex_unlock(&s.candado);
        if (cerrado) {
            liberar_jugador(&jugadores[a - 1]);
            liberar_jugador(&jugadores[b - 1]);
            break;
        }

        datos_mesas.emplace_back();
        Mesa& mesa = datos_mesas.back();
        mesa.servidor = &s;
        mesa.id_mesa = siguiente++;
        mesa.puma = &jugadores[a - 1];
        mesa.perros = &jugadores[b - 1];

        pthread_t h;
        if (pthread_create(&h, nullptr, hilo_mesa, &mesa) != 0) {
            registro_escribir("Error: no se pudo crear el hilo de la mesa\n");
            liberar_jugador(mesa.puma);
            liberar_jugador(mesa.perros);
            sem_post(&s.mesas);
            continue;
        }
        mesas.push_back(h);
    }

    for (pthread_t h : mesas) {
        pthread_join(h, nullptr);
    }
    for (int i = 0; i < n; i++) {
        liberar_jugador(&jugadores[i]);
    }
    for (pthread_t h : clientes) {
        pthread_join(h, nullptr);
    }

    senales_detener();

    registro_escribir(std::string(s.apagado ? "Torneo interrumpido: " : "Torneo finalizado: ") +
                      std::to_string(s.jugadas) + " partidas (" + std::to_string(s.abandonos) +
                      " por abandono), pico de mesas simultaneas " +
                      std::to_string(s.pico) + " (maximo " + std::to_string(m) + ")\n");

    for (int i = 0; i < n; i++) {
        pthread_cond_destroy(&jugadores[i].fin);
        pthread_mutex_destroy(&jugadores[i].candado);
    }
    sem_destroy(&s.mesas);
    pthread_cond_destroy(&s.apagar);
    pthread_cond_destroy(&s.llegada);
    pthread_mutex_destroy(&s.candado);
}
