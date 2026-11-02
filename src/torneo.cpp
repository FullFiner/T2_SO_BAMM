#include "torneo.hpp"

#include <pthread.h>
#include <semaphore.h>

#include <chrono>
#include <deque>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "juez.hpp"
#include "registro.hpp"

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
    std::deque<int> cola;
    pthread_mutex_t candado;
    pthread_cond_t llegada;
    sem_t mesas;
};

struct ArgsMesa {
    Servidor* servidor;
    int id_mesa;
    Jugador* puma;
    Jugador* perros;
};

static void liberar_jugador(Jugador* j) {
    pthread_mutex_lock(&j->candado);
    j->terminado = true;
    pthread_cond_broadcast(&j->fin);
    pthread_mutex_unlock(&j->candado);
}

static void* hilo_cliente(void* arg) {
    Jugador* j = static_cast<Jugador*>(arg);
    Servidor& s = *j->servidor;
    std::mt19937 rng(s.cfg.seed + 7919u * static_cast<unsigned>(j->id));
    int espera = std::uniform_int_distribution<int>(0, s.cfg.llegada_max_ms)(rng);
    std::this_thread::sleep_for(std::chrono::milliseconds(espera));

    pthread_mutex_lock(&s.candado);
    s.cola.push_back(j->id);
    s.llegados++;
    pthread_cond_signal(&s.llegada);
    pthread_mutex_unlock(&s.candado);

    registro_escribir("Jugador " + std::to_string(j->id) + " llega al torneo\n");

    pthread_mutex_lock(&j->candado);
    while (!j->terminado) {
        pthread_cond_wait(&j->fin, &j->candado);
    }
    pthread_mutex_unlock(&j->candado);

    registro_escribir("Jugador " + std::to_string(j->id) + " termina su participacion\n");
    return nullptr;
}

static void* hilo_mesa(void* arg) {
    ArgsMesa* a = static_cast<ArgsMesa*>(arg);
    Servidor& s = *a->servidor;

    pthread_mutex_lock(&s.candado);
    s.activas++;
    if (s.activas > s.pico) {
        s.pico = s.activas;
    }
    pthread_mutex_unlock(&s.candado);

    registro_escribir("[mesa " + std::to_string(a->id_mesa) + "] jugador " +
                      std::to_string(a->puma->id) + " (Puma) vs jugador " +
                      std::to_string(a->perros->id) + " (Perros)\n");
    juez_mesa(a->id_mesa, s.cfg);

    pthread_mutex_lock(&s.candado);
    s.activas--;
    s.jugadas++;
    pthread_mutex_unlock(&s.candado);

    liberar_jugador(a->puma);
    liberar_jugador(a->perros);
    sem_post(&s.mesas);
    return nullptr;
}

void torneo_ejecutar(int n, int m, const Config& cfg) {
    Servidor s;
    s.cfg = cfg;
    s.n = n;
    pthread_mutex_init(&s.candado, nullptr);
    pthread_cond_init(&s.llegada, nullptr);
    sem_init(&s.mesas, 0, static_cast<unsigned int>(m));

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

    std::deque<ArgsMesa> args_mesas;
    std::vector<pthread_t> mesas;
    int siguiente = 1;

    while (true) {
        pthread_mutex_lock(&s.candado);
        while (s.cola.size() < 2 && s.llegados < s.n) {
            pthread_cond_wait(&s.llegada, &s.candado);
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
        args_mesas.push_back({&s, siguiente++, &jugadores[a - 1], &jugadores[b - 1]});
        pthread_t h;
        if (pthread_create(&h, nullptr, hilo_mesa, &args_mesas.back()) != 0) {
            registro_escribir("Error: no se pudo crear el hilo de la mesa\n");
            liberar_jugador(&jugadores[a - 1]);
            liberar_jugador(&jugadores[b - 1]);
            sem_post(&s.mesas);
            continue;
        }
        mesas.push_back(h);
    }

    for (pthread_t h : mesas) {
        pthread_join(h, nullptr);
    }
    for (pthread_t h : clientes) {
        pthread_join(h, nullptr);
    }

    registro_escribir("Torneo finalizado: " + std::to_string(s.jugadas) +
                      " partidas, pico de mesas simultaneas " + std::to_string(s.pico) +
                      " (maximo " + std::to_string(m) + ")\n");

    for (int i = 0; i < n; i++) {
        pthread_cond_destroy(&jugadores[i].fin);
        pthread_mutex_destroy(&jugadores[i].candado);
    }
    sem_destroy(&s.mesas);
    pthread_cond_destroy(&s.llegada);
    pthread_mutex_destroy(&s.candado);
}