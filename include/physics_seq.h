#ifndef PHYSICS_SEQ_H
#define PHYSICS_SEQ_H

#include "body.h"

// Calcula la fuerza gravitacional neta (expresada como aceleracion ax, ay)
// sobre cada cuerpo debido a la atraccion de todos los demas cuerpos.
// Algoritmo O(N^2): por cada cuerpo i, se suma la contribucion de los N-1
// cuerpos restantes (Ley de Gravitacion Universal: F = G*m1*m2/r^2).
//
// Esta es la version SECUENCIAL — sirve como baseline de correctitud y
// de referencia para medir el speedup de la version paralela (Persona A,
// semana 2) en el mismo algoritmo.
//
// Entradas:
//   bodies    - arreglo de n cuerpos (solo lectura)
//   n         - cantidad de cuerpos
//   G         - constante gravitacional usada en la simulacion (ajustada
//               a las unidades de la simulacion, no la fisica real en SI)
//   softening - factor de suavizado (epsilon) que se suma a r^2 para evitar
//               division por cero o fuerzas explosivas cuando dos cuerpos
//               quedan muy cerca entre si
// Salidas:
//   ax, ay    - arreglos de tamano n (ya reservados por el caller) donde se
//               escribe la aceleracion resultante de cada cuerpo
void computeForcesSequential(const Body* bodies, int n, double G, double softening,
                              double* ax, double* ay);

#endif // PHYSICS_SEQ_H
