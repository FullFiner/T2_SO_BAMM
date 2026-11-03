#include "registro.hpp"

#include <pthread.h>

#include <fstream>
#include <iostream>

static pthread_mutex_t candado = PTHREAD_MUTEX_INITIALIZER;
static std::ofstream archivo;
static bool con_consola = false;

bool registro_abrir(const std::string& ruta, bool consola) {
    pthread_mutex_lock(&candado);
    archivo.open(ruta, std::ios::out | std::ios::trunc);
    bool ok = archivo.is_open();
    con_consola = consola;
    pthread_mutex_unlock(&candado);
    return ok;
}

void registro_escribir(const std::string& texto) {
    pthread_mutex_lock(&candado);
    if (archivo.is_open()) {
        archivo << texto;
        archivo.flush();
    }
    if (con_consola) {
        std::cout << texto;
        std::cout.flush();
    }
    pthread_mutex_unlock(&candado);
}

void registro_cerrar() {
    pthread_mutex_lock(&candado);
    if (archivo.is_open()) {
        archivo.close();
    }
    pthread_mutex_unlock(&candado);
}
