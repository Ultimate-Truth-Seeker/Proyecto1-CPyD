#ifndef FOGATA_PARTICLE_H
#define FOGATA_PARTICLE_H

#include <chrono>
#include <cstdint>
#include <vector>

#include "config.h"

class Rng {
 public:
    explicit Rng(uint32_t seed) : state_(seed ? seed : 0x9E3779B9u) {}

    uint32_t nextUint() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return state_;
    }
    float nextFloat() { return static_cast<float>(nextUint() >> 8) * (1.0f / 16777216.0f); }
    float range(float lo, float hi) { return lo + (hi - lo) * nextFloat(); }
    float gaussian() { return (nextFloat() + nextFloat() + nextFloat() - 1.5f) * 1.1547f; }

 private:
    uint32_t state_;
};

enum class ParticleKind : uint8_t { Flame, Ember, Smoke };

struct Particle {
    float x, y;
    float vx, vy;
    float temp;
    float coolRate;
    float radius;
    float phase;
    float tintR, tintG, tintB;
    ParticleKind kind;
};

class FireSystem {
 public:
    explicit FireSystem(const Config& cfg);

    // dt: delta time del frame.
    // outSparkIndex: indice del primer candidato critico encontrado este frame,
    //   o -1 si ninguno supera el umbral. En el build paralelo se calcula
    //   dentro de la misma region omp parallel que el physics, eliminando
    //   el overhead de lanzar una segunda region por frame.
    // outSparkIterations: particulas examinadas en la busqueda (para Anexo 3).
    void update(float dt, int& outSparkIndex, int& outSparkIterations);

    const std::vector<Particle>& particles() const { return particles_; }
    float hearthX()         const { return hearthX_; }
    float hearthY()         const { return hearthY_; }
    float hearthHalfWidth() const { return hearthHalfWidth_; }
    float scale()           const { return scale_; }
    float elapsed()         const { return elapsed_; }
    float flicker()         const;

    // Temperatura promedio de todas las particulas, calculada durante el
    // ultimo update() con una reduccion OpenMP explicita (ver particle.cpp).
    // Es una metrica agregada de memoria compartida: cada hilo acumula su
    // propia suma parcial y OpenMP las combina de forma segura al final de
    // la region paralela, sin necesidad de locks manuales.
    float averageTemperature()  const { return avgTemperature_; }
    int   lastSparkIndex()      const { return lastSparkIndex_; }
    int   lastSparkIterations() const { return lastSparkIterations_; }
    // Microsegundos que tomo SOLO la fase de busqueda en el ultimo update().
    // En el build paralelo se mide dentro de la region omp parallel (el timer
    // arranca antes de la fase 2 y para al salir); en el build secuencial
    // envuelve la llamada a findCriticalSpark().
    double lastSparkMicros()    const { return lastSparkMicros_; }

    // findCriticalSpark() ya no se llama desde fuera: la busqueda ocurre
    // dentro de update() en la misma region paralela que el physics.
    // Se mantiene como metodo privado para poder usarse en el build
    // secuencial (donde no hay fusion de regiones).
    // Si necesitas invocarla directamente en tests, hazla publica de nuevo.

 private:
    void respawn(Particle& particle);
    int  findCriticalSpark(int& outIterations) const;

    std::vector<Particle> particles_;
    Rng   rng_;
    float hearthX_;
    float hearthY_;
    float hearthHalfWidth_;
    float scale_;
    float wind_;
    float elapsed_ = 0.0f;
    float avgTemperature_ = 0.0f;
    // Resultados de la busqueda del ultimo update(), expuestos para el
    // renderer y el log sin necesidad de una segunda llamada publica.
    int    lastSparkIndex_      = -1;
    int    lastSparkIterations_ = 0;
    double lastSparkMicros_     = 0.0;
};

#endif  // FOGATA_PARTICLE_H