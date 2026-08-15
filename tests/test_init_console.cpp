// Entregable Semana 1 - Persona C
//
// Prueba de consola (sin graficos) que genera una galaxia con initGalaxy()
// y verifica visualmente por los valores impresos que:
//   1) el nucleo (cuerpo 0) esta en el centro del canvas con velocidad cero
//   2) las estrellas del disco tienen radio dentro del rango esperado
//      y velocidad tangencial (perpendicular al vector desde el centro)
//
// Compilar y correr: ver README.md / Makefile (target test_init)

#include <cstdio>
#include <cmath>
#include "body.h"
#include "init.h"

int main() {
    const int N = 8;
    const int WIDTH = 800;
    const int HEIGHT = 600;
    const double G = 1.0;
    const unsigned int SEED = 42;

    Body bodies[N];
    initGalaxy(bodies, N, WIDTH, HEIGHT, G, SEED);

    std::printf("=== Prueba de consola: generacion de galaxia inicial ===\n");
    std::printf("N=%d, canvas=%dx%d, G=%.2f, seed=%u\n\n", N, WIDTH, HEIGHT, G, SEED);

    double centerX = WIDTH / 2.0;
    double centerY = HEIGHT / 2.0;

    for (int i = 0; i < N; i++) {
        double dx = bodies[i].x - centerX;
        double dy = bodies[i].y - centerY;
        double distFromCenter = std::sqrt(dx * dx + dy * dy);
        double speed = std::sqrt(bodies[i].vx * bodies[i].vx + bodies[i].vy * bodies[i].vy);

        std::printf("Cuerpo %d: pos=(%.1f, %.1f)  vel=(%.2f, %.2f)  "
                    "masa=%.2f  dist_centro=%.1f  rapidez=%.2f  color=(%d,%d,%d)\n",
                    i, bodies[i].x, bodies[i].y, bodies[i].vx, bodies[i].vy,
                    bodies[i].mass, distFromCenter, speed,
                    bodies[i].r, bodies[i].g, bodies[i].b);
    }

    std::printf("\nVerificar: cuerpo 0 en el centro (400,300) con vel=(0,0) y masa alta;\n");
    std::printf("cuerpos 1-7 con distancias variadas y velocidad != 0 (orbitando).\n");
    return 0;
}
