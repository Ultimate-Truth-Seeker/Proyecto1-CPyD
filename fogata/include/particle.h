#ifndef FOGATA_PARTICLE_H
#define FOGATA_PARTICLE_H

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

    void update(float dt);

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
    float averageTemperature() const { return avgTemperature_; }

    // Busca la primera particula (en orden de indice) cuya temperatura
    // supera kCriticalSparkTemp -- una "chispa critica" que dispara un
    // destello visual en el render. Es una busqueda con corte temprano
    // (early-exit), no un recorrido completo: la secuencial se detiene en
    // cuanto encuentra el primer indice que cumple la condicion, y la
    // paralela reparte el recorrido entre hilos y corta apenas cualquiera
    // de ellos encuentra un candidato (ver particle.cpp para el mecanismo
    // de senalizacion). Retorna el indice encontrado, o -1 si ninguna
    // particula supera el umbral en este frame.
    //
    // outIterations recibe cuantas particulas se examinaron en total antes
    // de detenerse (sumando todos los hilos en la version paralela). Existe
    // solo para instrumentar el Anexo 3: permite reportar el ahorro de
    // trabajo real, no solo el tiempo de reloj.
    int findCriticalSpark(int& outIterations) const;

 private:
    void respawn(Particle& particle);

    std::vector<Particle> particles_;
    Rng   rng_;
    float hearthX_;
    float hearthY_;
    float hearthHalfWidth_;
    float scale_;
    float wind_;
    float elapsed_ = 0.0f;
    float avgTemperature_ = 0.0f;
};

#endif  // FOGATA_PARTICLE_H