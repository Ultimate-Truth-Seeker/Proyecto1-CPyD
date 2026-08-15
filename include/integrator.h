#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include "body.h"

// Actualiza velocidad y posicion de cada cuerpo dada su aceleracion,
// usando integracion de Euler semi-implicito (se actualiza la velocidad
// primero, y luego se usa la velocidad YA actualizada para mover la
// posicion). Es mas estable numericamente que Euler explicito puro y
// sigue siendo muy barato de calcular, lo cual importa porque este paso
// corre una vez por cuerpo, por frame.
//
// Entradas/Salidas:
//   bodies - arreglo de n cuerpos; se modifica in-place (posicion y velocidad)
//   ax, ay - aceleracion de cada cuerpo, ya calculada por computeForcesSequential
//            o su version paralela
//   n      - cantidad de cuerpos
//   dt     - paso de tiempo de la simulacion (segundos simulados por frame)
void integrateEuler(Body* bodies, const double* ax, const double* ay, int n, double dt);

#endif // INTEGRATOR_H
