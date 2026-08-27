// particle.h — Sistema de particulas de la fogata (modelo fisico).
#ifndef FOGATA_PARTICLE_H
#define FOGATA_PARTICLE_H

#include <cstdint>
#include <vector>

#include "config.h"

// Generador pseudoaleatorio xorshift32: rapido, sin estado global y con una
// instancia por sistema, de modo que la simulacion es reproducible al fijar
// la semilla con -s. Se usa para colores, tamanos y puntos de emision.
class Rng {
 public:
    explicit Rng(uint32_t seed) : state_(seed ? seed : 0x9E3779B9u) {}

    uint32_t nextUint() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return state_;
    }
    // Flotante uniforme en [0, 1).
    float nextFloat() { return static_cast<float>(nextUint() >> 8) * (1.0f / 16777216.0f); }
    // Flotante uniforme en [lo, hi).
    float range(float lo, float hi) { return lo + (hi - lo) * nextFloat(); }
    // Aproximacion barata a una normal centrada en 0 (suma de uniformes).
    float gaussian() { return (nextFloat() + nextFloat() + nextFloat() - 1.5f) * 1.1547f; }
 private:
    uint32_t state_;
};

// Una particula de fuego. La temperatura hace de "vida": cuando se enfria
// por debajo del umbral la particula se recicla en la base de la fogata,
// asi el arreglo tiene tamano fijo y no hay reservas de memoria por frame.
struct Particle {
    float x, y;        // posicion en pixeles (y crece hacia abajo)
    float vx, vy;      // velocidad en pixeles por segundo
    float temp;        // temperatura normalizada 1.0 = nucleo, 0.0 = apagada
    float coolRate;    // cuanta temperatura pierde por segundo
    float radius;      // radio del brillo que aporta al campo de calor
    float phase;       // desfase propio dentro del campo de turbulencia
    float tintR;       // tinte pseudoaleatorio (multiplica el color de cuerpo
    float tintG;       // negro para que no todas las llamas sean identicas)
    float tintB;
    bool  isEmber;     // true = chispa: mas pequena, mas rapida, se enfria lento
};

// Sistema de particulas de la fogata. Encapsula el estado fisico completo;
// no sabe nada de SDL ni de como se dibuja.
class FireSystem {
 public:
    explicit FireSystem(const Config& cfg);

    // Avanza la simulacion 'dt' segundos (integracion de Euler semi-implicita).
    void update(float dt);

    const std::vector<Particle>& particles() const { return particles_; }
    float hearthX()         const { return hearthX_; }
    float hearthY()         const { return hearthY_; }
    float hearthHalfWidth() const { return hearthHalfWidth_; }
    float elapsed()         const { return elapsed_; }
    // Intensidad de la brasa del hogar, late suavemente con el tiempo.
    float emberGlow()       const;

 private:
    // Reinicia una particula en la base de la fogata.
    void respawn(Particle& particle);

    std::vector<Particle> particles_;
    Rng      rng_;
    float    hearthX_;          // centro horizontal del hogar
    float    hearthY_;          // altura del lecho de brasas
    float    hearthHalfWidth_;  // medio ancho de la zona de emision
    float    scale_;            // factor de escala segun la resolucion
    float    wind_;             // fuerza del viento tomada de la config
    float    elapsed_ = 0.0f;   // tiempo simulado acumulado
};

#endif  // FOGATA_PARTICLE_H
