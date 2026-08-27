// particle.cpp — Fisica de la fogata: flotabilidad, arrastre y turbulencia.
//
// Modelo por particula (todas las fuerzas en pixeles/segundo^2):
//
//   a_y = -B * temp^1.5            flotabilidad: el aire caliente sube y la
//                                  fuerza crece con la temperatura
//       + T_y(x, y, t)             turbulencia (suma de senos y cosenos)
//       - D * v_y                  arrastre viscoso, proporcional a la velocidad
//
//   a_x = T_x(x, y, t)             turbulencia horizontal
//       + W * gust(t)              viento con rafagas periodicas
//       + C * (hearthX - x) * temp fuerza de confinamiento: estrecha la llama
//       - D * v_x                  arrastre viscoso
//
// La integracion es Euler semi-implicito (primero velocidad, luego posicion),
// estable para este rango de fuerzas y de coste O(N) por frame.
#include "particle.h"

#include <cmath>

namespace {

// --- Constantes fisicas del modelo, a 720 px de alto de referencia ---
constexpr float kBuoyancy      = 780.0f;  // empuje ascendente maximo
constexpr float kDragFlame     = 2.10f;   // arrastre de las llamas
constexpr float kDragEmber     = 0.85f;   // las chispas conservan mas inercia
constexpr float kTurbulence    = 300.0f;  // amplitud del campo turbulento
constexpr float kConfinement   = 1.60f;   // cuanto se estrecha la llama al subir
constexpr float kWindStrength  = 130.0f;  // escala del viento configurable
constexpr float kDeadTemp      = 0.03f;   // por debajo de esto la particula muere
constexpr float kEmberChance   = 0.06f;   // proporcion de chispas frente a llamas

// Rafagas de viento: tres senos de periodos incomensurables para que el
// patron nunca se repita de forma evidente. Devuelve aprox. [-1.75, 1.75].
float windGust(float t) {
    return std::sin(t * 0.37f)
         + 0.50f * std::sin(t * 0.91f + 1.7f)
         + 0.25f * std::sin(t * 2.30f + 0.4f);
}

}  // namespace

FireSystem::FireSystem(const Config& cfg)
    : rng_(cfg.seed),
      scale_(static_cast<float>(cfg.height) / 720.0f),
      wind_(cfg.wind) {
    hearthX_         = static_cast<float>(cfg.width) * 0.5f;
    hearthY_         = static_cast<float>(cfg.height) * 0.88f;
    hearthHalfWidth_ = static_cast<float>(cfg.height) * 0.085f;

    particles_.resize(static_cast<size_t>(cfg.nParticles));
    for (Particle& particle : particles_) {
        respawn(particle);
        // Escalonamos la temperatura inicial para que la llama aparezca ya
        // formada en el primer frame en lugar de brotar toda de golpe.
        particle.temp = rng_.nextFloat();
        particle.y   -= rng_.nextFloat() * static_cast<float>(cfg.height) * 0.45f;
    }
}

float FireSystem::emberGlow() const {
    // Latido lento del lecho de brasas, entre 0.75 y 1.0 aproximadamente.
    return 0.87f + 0.13f * std::sin(elapsed_ * 1.9f) * std::cos(elapsed_ * 0.7f);
}

void FireSystem::respawn(Particle& particle) {
    // Punto de emision: distribucion normal recortada sobre el lecho de brasas,
    // asi la llama nace densa en el centro y rala en los bordes.
    const float offset = rng_.gaussian() * hearthHalfWidth_ * 0.55f;
    particle.x = hearthX_ + offset;
    particle.y = hearthY_ - rng_.nextFloat() * 10.0f * scale_;

    particle.isEmber = (rng_.nextFloat() < kEmberChance);
    particle.phase   = rng_.range(0.0f, 6.2831853f);

    if (particle.isEmber) {
        // Chispas: diminutas, muy calientes, se enfrian lento y salen disparadas.
        particle.vx       = rng_.gaussian() * 28.0f * scale_;
        particle.vy       = -rng_.range(120.0f, 260.0f) * scale_;
        particle.temp     = rng_.range(0.90f, 1.00f);
        particle.coolRate = rng_.range(0.10f, 0.22f);
        particle.radius   = rng_.range(1.2f, 2.8f) * scale_;
        // Las chispas si toman colores saturados pseudoaleatorios: van del
        // ambar al rojo cereza pasando por el dorado.
        particle.tintR = rng_.range(1.00f, 1.20f);
        particle.tintG = rng_.range(0.55f, 1.00f);
        particle.tintB = rng_.range(0.10f, 0.45f);
    } else {
        // Llamas: masa de gas caliente, ascenso mas lento y vida corta.
        particle.vx       = rng_.gaussian() * 14.0f * scale_;
        particle.vy       = -rng_.range(40.0f, 110.0f) * scale_;
        particle.temp     = rng_.range(0.80f, 1.00f);
        particle.coolRate = rng_.range(0.45f, 0.95f);
        particle.radius   = rng_.range(7.0f, 20.0f) * scale_;
        // Variacion sutil de tinte para que la masa de fuego no se vea plana.
        particle.tintR = rng_.range(0.92f, 1.08f);
        particle.tintG = rng_.range(0.88f, 1.06f);
        particle.tintB = rng_.range(0.80f, 1.10f);
    }
}

void FireSystem::update(float dt) {
    elapsed_ += dt;
    const float t        = elapsed_;
    const float gust     = windGust(t) * wind_ * kWindStrength * scale_;
    const float turbAmp  = kTurbulence * scale_;
    const float ceilingY = -40.0f * scale_;  // margen sobre el borde superior

    for (Particle& p : particles_) {
        // --- Campo de turbulencia ---
        // Suma de senos y cosenos evaluada en la posicion de la particula: es
        // un campo continuo, asi que particulas vecinas se mueven de forma
        // coherente y se forman remolinos y lenguas de fuego reconocibles.
        const float turbX = std::sin(p.y * 0.014f + t * 1.70f + p.phase) *
                            std::cos(p.x * 0.011f - t * 1.10f);
        const float turbY = std::cos(p.x * 0.016f - t * 1.30f + p.phase) *
                            std::sin(p.y * 0.009f + t * 0.80f);

        const float drag = p.isEmber ? kDragEmber : kDragFlame;

        // --- Aceleracion resultante ---
        float ax = turbX * turbAmp * (0.35f + p.temp)
                 + gust
                 + (hearthX_ - p.x) * kConfinement * p.temp
                 - drag * p.vx;

        float ay = -kBuoyancy * scale_ * (p.temp * std::sqrt(p.temp))
                 + turbY * turbAmp * 0.45f
                 - drag * p.vy;

        // --- Integracion de Euler semi-implicita ---
        p.vx += ax * dt;
        p.vy += ay * dt;
        p.x  += p.vx * dt;
        p.y  += p.vy * dt;

        // --- Enfriamiento exponencial ---
        // La particula pierde calor mas rapido cuanto mas se aleja del hogar,
        // que es lo que hace que la punta de la llama se disuelva en humo.
        p.temp -= p.coolRate * dt * p.temp * 2.0f;

        // La llama se hace mas ancha y difusa conforme se enfria y se expande.
        if (!p.isEmber) p.radius += 9.0f * scale_ * dt;

        if (p.temp <= kDeadTemp || p.y < ceilingY) respawn(p);
    }
}
