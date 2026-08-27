# Fogata — Screensaver secuencial

Screensaver de una fogata renderizada con un sistema de particulas: cada llama
es una particula con temperatura propia que sube por flotabilidad, se enfria y
es empujada por un campo de turbulencia construido con sumas de senos y cosenos.
El color no se pinta directamente: se acumula *calor* en un campo escalar y
luego se mapea a una paleta de cuerpo negro (negro -> rojo -> naranja -> amarillo
-> blanco), que es lo que le da el aspecto de fuego real.

Esta es la **version secuencial** del Proyecto #1 de Computacion Paralela y
Distribuida. La version paralela con OpenMP se construye a partir de esta.

![Captura del screensaver](docs/captura.png)

## Dependencias

- Compilador C++17 (`g++`)
- SDL2

MSYS2 / MinGW64 (Windows):
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-pkgconf make
```

Ubuntu / Debian:
```bash
sudo apt-get install build-essential libsdl2-dev pkg-config
```

## Compilar y ejecutar

```bash
cd fogata
make
./build/fogata
```

## Parametros

Ningun valor esta fijo en el codigo: todo se lee de la linea de comandos.

| Opcion | Descripcion | Rango | Por defecto |
|---|---|---|---|
| `-n <entero>` | Cantidad de particulas (N) | 1 .. 2000000 | 3000 |
| `-w <entero>` | Ancho de la ventana | 640 .. 7680 | 1280 |
| `-h <entero>` | Alto de la ventana | 480 .. 4320 | 720 |
| `-i <decimal>` | Intensidad del brillo | 0.10 .. 5.00 | 1.00 |
| `-v <decimal>` | Viento lateral | -3.00 .. 3.00 | 0.35 |
| `-s <entero>` | Semilla pseudoaleatoria (0 = reloj) | >= 0 | 0 |
| `--no-vsync` | Desactiva la sincronia vertical | — | vsync activo |
| `--help` | Muestra la ayuda | — | — |

Ejemplos:

```bash
./build/fogata -n 8000 -w 1600 -h 900 -i 1.4
```

```bash
./build/fogata -n 20000 --no-vsync -v -1.2 -s 42
```

> Para medir FPS reales use `--no-vsync`: con vsync activo el contador queda
> topado al refresco del monitor (normalmente 60).

Salir: `ESC` o `Q`.

## Rendimiento medido (version secuencial)

Promedio de 7 mediciones de 1 segundo por corrida, con `--no-vsync` y semilla
fija, sobre un equipo con otras aplicaciones abiertas. Los FPS tambien se
imprimen por consola una vez por segundo, ademas de mostrarse en pantalla.

| N | Resolucion | FPS promedio |
|---|---|---|
| 1 | 1280x720 | 80.9 |
| 3000 (por defecto) | 1280x720 | 41.4 |
| 3000 | 640x480 | 99.3 |
| 8000 | 1280x720 | 27.8 |
| 16000 | 1280x720 | 20.5 |

El coste crece de forma practicamente lineal con N, que es justo lo que se
busca de cara a la version paralela: el bucle de acumulacion de particulas es
el que domina el tiempo de frame y es el candidato natural a paralelizar.

## Programacion defensiva

Todo argumento se valida antes de tocar SDL: se rechazan valores no numericos
(`-n abc`), opciones sin valor (`-n` al final), opciones desconocidas y
cualquier numero fuera de rango. En todos esos casos el programa imprime el
motivo, muestra el modo de uso y termina con codigo de salida 1 sin reservar
memoria ni abrir ventana.
