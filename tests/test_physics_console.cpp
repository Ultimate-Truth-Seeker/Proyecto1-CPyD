// Entregable Semana 1 - Persona A
//
// Prueba de consola (sin graficos) para verificar que el calculo de fuerzas
// secuencial y el integrador funcionan correctamente juntos. Se usan 3
// cuerpos con valores conocidos (un cuerpo central pesado + dos "satelites"
// livianos) para poder revisar a simple vista que:
//   1) el cuerpo central casi no se mueve (su masa es mucho mayor)
//   2) los satelites son atraidos hacia el centro y cambian de velocidad
//
// Compilar y correr: ver README.md / Makefile (target test_physics)

#include <cstdio>
#include "body.h"
#include "physics_seq.h"
#include "integrator.h"

int main() {
    const int N = 3;
    const double G = 1.0;
    const double SOFTENING = 0.1;
    const double DT = 0.01;
    const int STEPS = 5;

    Body bodies[N];

    // Cuerpo 0: nucleo central, pesado, en el origen, sin velocidad inicial
    bodies[0] = {0.0, 0.0, 0.0, 0.0, 1000.0, 255, 255, 0, 8.0};

    // Cuerpo 1: satelite liviano a la derecha del nucleo
    bodies[1] = {50.0, 0.0, 0.0, 4.0, 1.0, 0, 255, 255, 3.0};

    // Cuerpo 2: satelite liviano arriba del nucleo
    bodies[2] = {0.0, 50.0, -4.0, 0.0, 1.0, 255, 0, 255, 3.0};

    double ax[N], ay[N];

    std::printf("=== Prueba de consola: fisica secuencial + integrador ===\n");
    std::printf("N=%d, G=%.2f, softening=%.2f, dt=%.3f\n\n", N, G, SOFTENING, DT);

    for (int step = 0; step <= STEPS; step++) {
        std::printf("--- Paso %d ---\n", step);
        for (int i = 0; i < N; i++) {
            std::printf("  Cuerpo %d: pos=(%.3f, %.3f)  vel=(%.3f, %.3f)\n",
                        i, bodies[i].x, bodies[i].y, bodies[i].vx, bodies[i].vy);
        }

        computeForcesSequential(bodies, N, G, SOFTENING, ax, ay);
        integrateEuler(bodies, ax, ay, N, DT);
    }

    std::printf("\nPrueba completada: si los satelites cambiaron de posicion y\n");
    std::printf("velocidad hacia el nucleo, la fisica + integrador funcionan.\n");
    return 0;
}
