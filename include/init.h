#ifndef INIT_H
#define INIT_H

#include "body.h"

// Genera la distribucion inicial de cuerpos en forma de galaxia: un cuerpo
// central masivo (nucleo) en el centro del canvas, y n-1 cuerpos livianos
// distribuidos en un disco alrededor de el, cada uno con velocidad
// tangencial inicial calculada para aproximar una orbita circular estable
// (v = sqrt(G * M_nucleo / r)). Ver docs/design_galaxy_distribution.md
// para la justificacion del modelo.
//
// Entradas:
//   n             - cantidad total de cuerpos a generar (incluye el nucleo, n >= 1)
//   width, height - dimensiones del canvas; se usan para centrar el nucleo
//                   y escalar el radio del disco de forma proporcional
//   G             - constante gravitacional usada en la simulacion (debe
//                   coincidir con la G usada luego en computeForcesSequential)
//   seed          - semilla para el generador pseudoaleatorio (reproducibilidad
//                   de la distribucion entre corridas, util para benchmarking)
// Salida:
//   bodies        - arreglo de tamano n, YA RESERVADO por el caller (ej. con
//                   `new Body[n]` en main.cpp), que esta funcion llena con las
//                   posiciones/velocidades/masas/colores iniciales
void initGalaxy(Body* bodies, int n, int width, int height, double G, unsigned int seed);

#endif // INIT_H
