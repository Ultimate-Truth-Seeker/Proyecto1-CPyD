#include "integrator.h"

void integrateEuler(Body* bodies, const double* ax, const double* ay, int n, double dt) {
    for (int i = 0; i < n; i++) {
        // 1. Actualizar velocidad con la aceleracion actual: v = v + a*dt
        bodies[i].vx += ax[i] * dt;
        bodies[i].vy += ay[i] * dt;

        // 2. Actualizar posicion con la velocidad YA actualizada: x = x + v*dt
        //    (esto es lo que hace el metodo "semi-implicito", tambien
        //    llamado Euler-Cromer, y es mas estable que usar la velocidad vieja)
        bodies[i].x += bodies[i].vx * dt;
        bodies[i].y += bodies[i].vy * dt;
    }
}
