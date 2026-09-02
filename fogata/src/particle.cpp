#include "particle.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kBuoyancy      = 700.0f;
constexpr float kDragFlame     = 2.10f;
constexpr float kDragEmber     = 0.85f;
constexpr float kDragSmoke     = 1.35f;
constexpr float kTurbulence    = 330.0f;
constexpr float kConfinement   = 2.40f;
constexpr float kWindStrength  = 130.0f;
constexpr float kDeadTemp      = 0.10f;
constexpr float kEmberChance   = 0.030f;
constexpr float kSmokeChance   = 0.042f;
constexpr float kMaxFlameRadius = 14.0f;
constexpr float kMaxSmokeRadius = 26.0f;

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
    hearthY_         = static_cast<float>(cfg.height) * 0.86f;
    hearthHalfWidth_ = static_cast<float>(cfg.height) * 0.115f;

    particles_.resize(static_cast<size_t>(cfg.nParticles));
    for (Particle& particle : particles_) {
        respawn(particle);
        particle.temp = rng_.nextFloat();
        particle.y   -= rng_.nextFloat() * static_cast<float>(cfg.height) * 0.45f;
    }
}

float FireSystem::flicker() const {
    return 0.84f + 0.10f * std::sin(elapsed_ * 5.7f)
                 + 0.06f * std::sin(elapsed_ * 13.1f + 1.3f)
                 + 0.04f * std::sin(elapsed_ * 2.3f + 0.7f);
}

void FireSystem::respawn(Particle& particle) {
    const float roll = rng_.nextFloat();
    particle.kind = (roll < kEmberChance)                 ? ParticleKind::Ember
                  : (roll < kEmberChance + kSmokeChance)  ? ParticleKind::Smoke
                                                          : ParticleKind::Flame;

    const float offset = rng_.gaussian() * hearthHalfWidth_ * 0.80f;
    particle.x     = hearthX_ + offset;
    particle.y     = hearthY_ - rng_.nextFloat() * 10.0f * scale_;
    particle.phase = rng_.range(0.0f, 6.2831853f);

    switch (particle.kind) {
        case ParticleKind::Ember:
            particle.vx       = rng_.gaussian() * 34.0f * scale_;
            particle.vy       = -rng_.range(80.0f, 165.0f) * scale_;
            particle.temp     = rng_.range(0.90f, 1.00f);
            particle.coolRate = rng_.range(0.45f, 0.80f);
            particle.radius   = rng_.range(1.0f, 2.2f) * scale_;
            particle.tintR    = rng_.range(1.00f, 1.20f);
            particle.tintG    = rng_.range(0.55f, 1.00f);
            particle.tintB    = rng_.range(0.10f, 0.45f);
            break;

        case ParticleKind::Smoke:
            particle.x       += rng_.gaussian() * hearthHalfWidth_ * 0.30f;
            particle.y       -= rng_.range(0.0f, 60.0f) * scale_;
            particle.vx       = rng_.gaussian() * 10.0f * scale_;
            particle.vy       = -rng_.range(25.0f, 60.0f) * scale_;
            particle.temp     = rng_.range(0.16f, 0.30f);
            particle.coolRate = rng_.range(0.06f, 0.14f);
            particle.radius   = rng_.range(9.0f, 17.0f) * scale_;
            particle.tintR    = rng_.range(0.85f, 1.05f);
            particle.tintG    = rng_.range(0.85f, 1.05f);
            particle.tintB    = rng_.range(0.90f, 1.15f);
            break;

        case ParticleKind::Flame:
            particle.vx       = rng_.gaussian() * 14.0f * scale_;
            particle.vy       = -rng_.range(40.0f, 110.0f) * scale_;
            particle.temp     = rng_.range(0.80f, 1.00f);
            particle.coolRate = rng_.range(0.40f, 0.85f);
            particle.radius   = rng_.range(4.0f, 10.0f) * scale_;
            particle.tintR    = rng_.range(0.92f, 1.08f);
            particle.tintG    = rng_.range(0.88f, 1.06f);
            particle.tintB    = rng_.range(0.80f, 1.10f);
            break;
    }
}

void FireSystem::update(float dt) {
    elapsed_ += dt;
    const float t        = elapsed_;
    const float gust     = windGust(t) * wind_ * kWindStrength * scale_;
    const float turbAmp  = kTurbulence * scale_;
    const float ceilingY = -60.0f * scale_;

    for (Particle& p : particles_) {
        const float turbX = std::sin(p.y * 0.021f + t * 1.90f + p.phase) *
                            std::cos(p.x * 0.017f - t * 1.15f);
        const float turbY = std::cos(p.x * 0.024f - t * 1.40f + p.phase) *
                            std::sin(p.y * 0.013f + t * 0.85f);

        const bool isSmoke = (p.kind == ParticleKind::Smoke);
        const float drag = (p.kind == ParticleKind::Ember) ? kDragEmber
                         : isSmoke                         ? kDragSmoke
                                                           : kDragFlame;
        const float lift = isSmoke ? 190.0f : kBuoyancy * (p.temp * std::sqrt(p.temp));

        p.vx += (turbX * turbAmp * (isSmoke ? 0.55f : 0.35f + p.temp)
              + gust * (isSmoke ? 2.20f : 1.0f)
              + (hearthX_ - p.x) * kConfinement * (1.0f - p.temp) * (isSmoke ? 0.15f : 1.0f)
              - drag * p.vx) * dt;

        p.vy += (-lift * scale_
              + turbY * turbAmp * 0.45f
              - drag * p.vy) * dt;

        p.x += p.vx * dt;
        p.y += p.vy * dt;

        p.temp -= p.coolRate * dt * p.temp * 2.0f;

        if (isSmoke) {
            p.radius = std::min(p.radius + 15.0f * scale_ * dt, kMaxSmokeRadius * scale_);
        } else if (p.kind == ParticleKind::Flame) {
            p.radius = std::min(p.radius + 4.0f * scale_ * dt, kMaxFlameRadius * scale_);
        }

        const float deadTemp = isSmoke ? 0.035f : kDeadTemp;
        if (p.temp <= deadTemp || p.y < ceilingY) respawn(p);
    }
}
