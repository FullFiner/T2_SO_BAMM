#pragma once

bool senales_iniciar(void (*al_interrumpir)(void*), void* contexto);

void senales_detener();
