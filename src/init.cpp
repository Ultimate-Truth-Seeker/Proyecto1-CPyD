#include "init.h"
#include <cmath>
#include <cstdlib>

// Fraccion del lado mas corto del canvas donde empieza y termina el disco
// de estrellas (deja un hueco vacio cerca del nucleo, ver docs/design_galaxy_distribution.md)
static const double DISK_R_MIN_FRACTION = 0.05;
static const double DISK_R_MAX_FRACTION = 0.45;

// Masa relativa del nucleo respecto a la masa promedio de una estrella del disco
static const double CORE_MASS_MULTIPLIER = 8000.0;

static double randomInRange(double lo, double hi) {
    double t = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
    return lo + t * (hi - lo);
}

void initGalaxy(Body* bodies, int n, int width, int height, double G, unsigned int seed) {
    srand(seed);

    double centerX = width / 2.0;
    double centerY = height / 2.0;
    double shortSide = (width < height) ? width : height;

    double rMin = shortSide * DISK_R_MIN_FRACTION;
    double rMax = shortSide * DISK_R_MAX_FRACTION;

    // Masa base de una estrella "tipica" del disco; el nucleo es ordenes de
    // magnitud mas pesado para dominar la dinamica orbital del sistema.
    const double baseStarMass = 1.0;
    const double coreMass = baseStarMass * CORE_MASS_MULTIPLIER;

    // Cuerpo 0: nucleo central, en reposo en el centro del canvas.
    bodies[0].x = centerX;
    bodies[0].y = centerY;
    bodies[0].vx = 0.0;
    bodies[0].vy = 0.0;
    bodies[0].mass = coreMass;
    bodies[0].radius = 10.0;
    bodies[0].r = 255;
    bodies[0].g = 220;
    bodies[0].b = 120;  // nucleo con tono dorado, distinguible del resto

    // Cuerpos 1..n-1: estrellas del disco en orbita alrededor del nucleo.
    for (int i = 1; i < n; i++) {
        double radius = randomInRange(rMin, rMax);
        double theta = randomInRange(0.0, 2.0 * M_PI);

        bodies[i].x = centerX + radius * std::cos(theta);
        bodies[i].y = centerY + radius * std::sin(theta);

        // Velocidad orbital circular aproximada: v = sqrt(G * M_nucleo / r)
        // (se despeja de igualar fuerza gravitacional con fuerza centripeta)
        double orbitalSpeed = std::sqrt(G * coreMass / radius);

        // Vector tangencial: rotar 90 grados el vector radial unitario
        bodies[i].vx = -orbitalSpeed * std::sin(theta);
        bodies[i].vy = orbitalSpeed * std::cos(theta);

        bodies[i].mass = randomInRange(baseStarMass * 0.5, baseStarMass * 2.0);
        bodies[i].radius = 2.0;

        assignRandomColor(bodies[i]);
    }
}
