# Tarea 2 SO - Torneo de Komikan concurrente

Benjamín Aguilera y Matías Marchant. Sistemas Operativos

Simulación de un torneo de Komikan (1 puma contra 12 perros) donde cada ficha es un hilo y se mueven todas al mismo tiempo, sin turnos. Un servidor recibe N jugadores, los junta de a dos y los reparte en máximo M mesas. Usamos pthreads (mutex y variables de condición) y un semáforo POSIX para las mesas.

## Compilar

```
make
```

Compila con `g++ -Wall -Wextra -std=c++17 -pthread`. Para borrar lo compilado: `make clean`.

## Ejecutar

```
./torneo N M [config.txt]
```

- N: cantidad de jugadores (1 a 10000)
- M: máximo de mesas al mismo tiempo (1 a 1000)
- el config es opcional, sin él se usan los valores por defecto

Ejemplo: `./torneo 6 2 config.txt`

Lo que pasa en las partidas queda en `torneo.log` y, si `log_consola=1`, también en la terminal. El tablero se imprime al empezar, cada 10 movimientos y al final de cada partida (`P` puma, `o` perro, `.` vacío). Si N es impar, el último jugador se queda sin pareja y el programa lo avisa.

## Config

Líneas `clave=valor`. Las que empiezan con `#` son comentarios. Si una línea está mal, el programa dice cuál es y no parte.

- `delay_min_ms`, `delay_max_ms`: retardo de cada movimiento (50 y 300 por defecto)
- `seed`: semilla para repetir una prueba (si no se pone, es al azar)
- `max_movimientos`: tope de movimientos, si se llega es empate (2000 por defecto)
- `llegada_max_ms`: tiempo máximo que tarda en llegar cada jugador
- `log_consola`: 1 para ver el log en la terminal, 0 para que solo vaya al archivo
- `prob_abandono`: porcentaje de mesas donde un jugador se cae a mitad de partida (0 a 100)
- `puma` y `perros`: posición inicial de las fichas (por defecto el puma en el trono y los perros del 13 al 24)

Archivos de ejemplo: `config.txt` (normal), `config_prueba.txt` (rápido), `config_abandono.txt` (siempre hay abandono) y `config_disposicion.txt` (otra posición inicial).

## Tablero

Son 28 nodos: la cuadrícula de 5x5 y los 3 del triángulo. El 27 es el trono.

```
               27
         25          26
     0 -  1 -  2 -  3 -  4
     |    |    |    |    |
     5 -  6 -  7 -  8 -  9
     |    |    |    |    |
    10 - 11 - 12 - 13 - 14
     |    |    |    |    |
    15 - 16 - 17 - 18 - 19
     |    |    |    |    |
    20 - 21 - 22 - 23 - 24
```

Líneas del triángulo:

```
27 - 25 - 0
27 - 26 - 4
27 - 2
25 - 26
25 - 1
26 - 3
```

Las fichas se mueven por filas, columnas y las líneas del triángulo. No pusimos diagonales en la cuadrícula.

## Cómo funciona

- **Partida:** cada mesa tiene un mutex que protege su tablero. Una ficha espera su retardo, toma el mutex, revisa qué casillas tiene libres, se mueve y lo suelta. Así dos fichas no pueden quedarse con la misma casilla.
- **Servidor:** los jugadores llegan a una cola y el servidor los saca de a dos. Un semáforo que parte en M controla las mesas: si están todas ocupadas, el par que sigue espera bloqueado hasta que una mesa se libere.
- **Juez:** cada mesa tiene un hilo juez que prepara el tablero, anota los eventos, ve quién gana y al final libera la mesa.
- **Abandono:** con `prob_abandono` una ficha muere teniendo el mutex tomado. Una función registrada con `pthread_cleanup_push` suelta el mutex y le da la victoria al otro jugador.
- **Ctrl+C:** un hilo aparte espera la señal. Cuando llega, ya no entran jugadores nuevos, se avisa a las partidas activas que terminen y se libera todo antes de salir.

El detalle del diseño y el análisis de deadlock, livelock e inanición están en el informe PDF.

## Archivos

- `main.cpp`: lee los argumentos y parte el torneo
- `config.cpp`: lee y valida el config
- `tablero.cpp`: conexiones entre los nodos del tablero
- `juego.cpp`: reglas (mover, capturar, ver quién gana)
- `partida.cpp`: los 13 hilos de cada partida
- `juez.cpp`: el juez de cada mesa
- `torneo.cpp`: servidor, jugadores y mesas
- `senales.cpp`: manejo del Ctrl+C
- `registro.cpp`: el log, compartido por todos los hilos

## Pruebas

Lo probamos con varias semillas y con Ctrl+C en distintos momentos, usando Valgrind, ThreadSanitizer y AddressSanitizer. No salieron fugas ni avisos. Por ejemplo:

```
valgrind --leak-check=full ./torneo 4 2 config_prueba.txt
```

Para ver un abandono: `./torneo 4 2 config_abandono.txt`

## Limitaciones

- Las fichas se mueven al azar, sin estrategia, por eso algunas partidas terminan en empate por `max_movimientos`.
- El abandono es simulado, no hay jugadores reales que se desconecten.
- Todo corre en un solo proceso con hilos, no usamos memoria compartida entre procesos.