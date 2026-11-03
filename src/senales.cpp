#include "senales.hpp"

#include <pthread.h>
#include <signal.h>

static pthread_t hilo;
static bool activo = false;
static void (*callback)(void*) = nullptr;
static void* ctx = nullptr;

static void llenar_conjunto(sigset_t& conjunto) {
    sigemptyset(&conjunto);
    sigaddset(&conjunto, SIGINT);
    sigaddset(&conjunto, SIGTERM);
    sigaddset(&conjunto, SIGUSR1);
}

static void* hilo_senales(void*) {
    sigset_t conjunto;
    llenar_conjunto(conjunto);

    while (true) {
        int senal = 0;
        if (sigwait(&conjunto, &senal) != 0) {
            return nullptr;
        }
        if (senal == SIGUSR1) {
            return nullptr;
        }
        callback(ctx);
    }
}

bool senales_iniciar(void (*al_interrumpir)(void*), void* contexto) {
    callback = al_interrumpir;
    ctx = contexto;

    sigset_t conjunto;
    llenar_conjunto(conjunto);
    if (pthread_sigmask(SIG_BLOCK, &conjunto, nullptr) != 0) {
        return false;
    }
    if (pthread_create(&hilo, nullptr, hilo_senales, nullptr) != 0) {
        return false;
    }
    activo = true;
    return true;
}

void senales_detener() {
    if (!activo) {
        return;
    }
    pthread_kill(hilo, SIGUSR1);
    pthread_join(hilo, nullptr);
    activo = false;
}
