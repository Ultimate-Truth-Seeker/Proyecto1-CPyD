#include "physics_seq.h"
#include <cmath>

void computeForcesSequential(const Body* bodies, int n, double G, double softening,
                              double* ax, double* ay) {
    for (int i = 0; i < n; i++) {
        double fx = 0.0;
        double fy = 0.0;

        for (int j = 0; j < n; j++) {
            if (i == j) continue;

            double dx = bodies[j].x - bodies[i].x;
            double dy = bodies[j].y - bodies[i].y;

            // softening^2 evita division por cero cuando dos cuerpos coinciden
            // o quedan extremadamente cerca (evita "explosiones" numericas).
            double distSq = dx * dx + dy * dy + softening * softening;
            double dist = std::sqrt(distSq);

            // Ley de gravitacion universal: F = G * m_i * m_j / r^2
            double forceMag = (G * bodies[i].mass * bodies[j].mass) / distSq;

            // Descomposicion del vector fuerza en componentes x/y
            // (trigonometria via el vector unitario dx/dist, dy/dist).
            fx += forceMag * (dx / dist);
            fy += forceMag * (dy / dist);
        }

        // a = F / m
        ax[i] = fx / bodies[i].mass;
        ay[i] = fy / bodies[i].mass;
    }
}
