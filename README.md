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

Las claves que no aparecen quedan con su valor por defecto. Ejemplo de un config:

```
# retardos de cada ficha
delay_min_ms=50
delay_max_ms=300
seed=12345
max_movimientos=2000
log_consola=1
prob_abandono=0
puma=27
perros=13,14,15,16,17,18,19,20,21,22,23,24
```

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

## Sincronización

**Un mutex por mesa.** Cada mesa tiene su tablero y un solo mutex que lo protege. Una ficha toma el mutex, revisa qué casillas tiene libres, se mueve y lo suelta. Como revisar y mover pasa con el mutex tomado, dos fichas no pueden quedarse con la misma casilla: la que llega segunda ya la ve ocupada. No usamos un mutex por casilla porque una captura toca tres casillas a la vez, y con varios candados habría que cuidar el orden en que se toman para no caer en un deadlock. Con uno solo ese problema no existe. Lo malo es que dentro de una mesa se mueve una ficha a la vez, pero mover dura muy poco comparado con los 50 a 300 ms que espera cada ficha, así que casi no se nota. Las mesas no comparten tablero, así que entre mesas distintas no se frenan.

**Espera con variable de condición.** El retardo de cada ficha se hace con `pthread_cond_timedwait` y no con sleep. Mientras espera suelta el mutex para que las otras fichas puedan moverse, y si la partida termina o llega un Ctrl+C se despierta altiro en vez de seguir durmiendo.

**Semáforo de mesas.** El semáforo parte en M. Antes de crear una mesa el servidor hace `sem_wait`, y cuando una partida termina la mesa hace `sem_post`. Si las M mesas están ocupadas, el servidor se queda bloqueado en el `sem_wait` sin gastar CPU, que es la espera pasiva que pide el enunciado.

**Cola de jugadores.** Los jugadores llegan a una cola protegida por el mutex del servidor y avisan con una variable de condición. El servidor saca de a dos en orden de llegada (el primero es puma y el segundo perros), así que nadie se queda esperando para siempre. Cada jugador espera en su propia variable de condición hasta que su partida termina.

**Juez.** Cada mesa tiene un hilo juez que prepara el tablero y sus primitivas, anota los eventos, ve quién gana, espera a los 13 hilos con `pthread_join` y al final destruye el mutex y la variable de condición.

**Abandono.** Con `prob_abandono` una ficha muere (`pthread_exit`) teniendo el mutex tomado, lo que dejaría la mesa trancada. Para eso cada ficha registra con `pthread_cleanup_push` una función que se ejecuta cuando el hilo muere: anota el abandono, le da la victoria al otro jugador y suelta el mutex.

**Ctrl+C.** Las señales están bloqueadas en todos los hilos menos en uno que hace `sigwait`. Lo hicimos así porque dentro de un manejador de señales normal no se puede tomar un mutex de forma segura. Cuando llega la señal, ese hilo marca el servidor como apagado, avisa a las partidas activas y hace un `sem_post` por si el servidor estaba esperando una mesa. Después se hace join de todos los hilos y se destruye todo antes de salir.

**Log.** Todos los hilos escriben en el mismo log, así que tiene su propio mutex para que las líneas no se mezclen.

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