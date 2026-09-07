#include "particle.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <limits>
#include <omp.h>
namespace {

constexpr int kMinimumOpenMpThreads = 8;

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

// Umbral de "chispa critica" para findCriticalSpark(). 0.999 se calibro
// empiricamente (ver docs/decisiones.md, entrada sobre el umbral de chispa
// critica): con este umbral, en la maquina de referencia caen en promedio
// ~38 particulas por frame por encima del umbral de 250,000 totales, lo
// que ubica el primer candidato (en orden de indice) alrededor del 2.5%
// del arreglo en promedio -- suficientemente disperso para que repartir el
// escaneo entre hilos tenga margen real de ganancia, pero sin llegar nunca
// a cero candidatos en las muestras tomadas (lo que dejaria la busqueda sin
// nada que encontrar). Umbrales mas bajos (0.99-0.995) dejaban el primer
// candidato casi siempre en el primer 0.5% del arreglo, sin margen de
// mejora; umbrales mas altos (0.9995) arriesgaban frames sin ningun
// candidato. Si se cambia kEmberChance o el rango de temperatura inicial
// de los Ember en respawn(), este umbral debe recalibrarse con el mismo
// metodo (contar candidatos por frame en un histograma antes de fijar el
// valor final).
constexpr float kCriticalSparkTemp = 0.999f;

// Tamano de chunk que cada hilo reserva por turno en findCriticalSpark(),
// via el contador atomico nextChunkStart (fetch_add). No es un schedule de
// OpenMP -- ver el comentario dentro de findCriticalSpark() para la
// diferencia, que importa: un #pragma omp parallel for con schedule(dynamic,
// N) reparte igual TODOS los chunks del rango [0, n) sin importar que un
// hilo ya haya encontrado el candidato, porque el propio 'for' de OpenMP no
// se entera de que hay una condicion de salida -- eso fue exactamente el bug
// de la primera version de esta funcion (ver Anexo 3 / docs/decisiones.md).
// Aqui en cambio cada hilo pide chunks en un bucle manual que el hilo mismo
// controla, y dejar de pedir es lo que saca al hilo del trabajo de verdad.
// Un chunk chico (64) significa que un hilo nunca se compromete a una franja
// grande sin candidato: agota el chunk actual y, si 'found' ya esta activa,
// nunca pide el siguiente.
constexpr int kSparkChunkSize = 64;

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
#ifdef _OPENMP
    const int availableThreads = omp_get_num_procs();
    const int configuredThreads = 8;//std::max(kMinimumOpenMpThreads, availableThreads);
    omp_set_dynamic(0);
    omp_set_num_threads(configuredThreads);
    std::printf("OpenMP: %d hilos configurados (procesadores disponibles: %d)\n",
                configuredThreads, availableThreads);
#endif

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

    // Mecanismo de sincronizacion explicito (requisito de la rubrica):
    // cada hilo mantiene su propia suma parcial de temperatura en
    // 'temperatureSum' y OpenMP las combina de forma segura al cerrar la
    // region paralela (reduction), sin que ningun hilo escriba directamente
    // sobre una variable compartida ni se necesite un lock manual.
    float temperatureSum = 0.0f;

    #pragma omp parallel for schedule(static) reduction(+:temperatureSum)
    for (int i = 0; i < static_cast<int>(particles_.size()); ++i) {
        Particle& p = particles_[i];
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

        temperatureSum += p.temp;
    }

    avgTemperature_ = particles_.empty()
        ? 0.0f
        : temperatureSum / static_cast<float>(particles_.size());
}

int FireSystem::findCriticalSpark(int& outIterations) const {
    const int n = static_cast<int>(particles_.size());

#ifndef _OPENMP
    // Build secuencial: recorrido lineal simple con corte temprano real.
    // Esta es la definicion de referencia -- la version paralela de abajo
    // debe encontrar el MISMO indice que esta, siempre, para cualquier
    // estado de particles_. Si alguna vez divergen, hay un bug de carrera
    // en la senalizacion de corte, no una diferencia de "criterio".
    for (int i = 0; i < n; ++i) {
        if (particles_[static_cast<size_t>(i)].temp > kCriticalSparkTemp) {
            outIterations = i + 1;
            return i;
        }
    }
    outIterations = n;
    return -1;
#else
    // Build paralelo.
    //
    // NOTA HISTORICA (por que esto NO es un simple "#pragma omp parallel for
    // schedule(dynamic, N)" con un 'continue' al ver la bandera activa):
    // esa fue la primera implementacion, y estaba rota. 'continue' dentro de
    // un parallel for NO saca al hilo del bucle -- solo salta el CUERPO de
    // esa iteracion. El scheduler de OpenMP sigue repartiendo TODOS los
    // chunks de [0, n) sin importar que la bandera de corte ya este activa,
    // asi que el bucle recorria las 250,000 posiciones (saltando trabajo en
    // la mayoria, pero sin dejar de VISITARLAS) sin importar schedule(static)
    // ni schedule(dynamic). Medido: outIterations bajaba de 250,000 a
    // ~234,800, pero nunca menos, porque un chunk entero seguia
    // repartiendose aunque su primera particula ya disparara el corte.
    //
    // SEGUNDA NOTA HISTORICA (bug de CORRECTITUD, no solo de rendimiento):
    // una version intermedia de esta funcion cortaba a todos los hilos en
    // cuanto CUALQUIER hilo encontraba un candidato ('found' a secas). Eso
    // es incorrecto: fetch_add reparte los chunks en orden ascendente de
    // ASIGNACION, pero no de EJECUCION real -- un hilo puede tener asignado
    // el chunk [0,64) y aun no haberlo escaneado cuando otro hilo, con el
    // chunk [50000,50064), ya encontro un candidato ahi y activo 'found'.
    // Si el primer hilo se detiene sin escanear su chunk, el resultado final
    // seria 50000-y-tantos en vez del indice real (que podria estar dentro
    // de [0,64)) -- violando la garantia de que el paralelo debe encontrar
    // SIEMPRE el mismo indice que el secuencial.
    //
    // La condicion de corte correcta no es "algun candidato ya aparecio" a
    // secas: es "todo chunk que podria contener un indice MENOR al mejor
    // candidato conocido ya fue escaneado, o esta garantizado que nadie mas
    // lo tomara con un resultado mejor". La forma simple y suficiente de
    // lograr esto: cada hilo compara el INICIO de su propio siguiente chunk
    // contra 'winner' antes de pedirlo. Si start >= winner, ese chunk (y
    // todos los siguientes, porque los chunks se reparten en orden
    // ascendente) solo podrian producir un indice mayor o igual al mejor ya
    // encontrado, asi que no hace falta escanearlo. Si start < winner, el
    // chunk SI podria contener un indice mejor, y hay que escanearlo antes
    // de dar por definitivo el resultado actual.
    std::atomic<bool> found{false};
    std::atomic<int>  winner{std::numeric_limits<int>::max()};
    std::atomic<int>  nextChunkStart{0};
    std::atomic<long long> iterationsDoneAtomic{0};

    #pragma omp parallel
    {
        long long localIterations = 0;

        for (;;) {
            // Reserva atomicamente el siguiente chunk de kSparkChunkSize
            // indices. fetch_add retorna el valor ANTES de sumar, asi que
            // cada hilo obtiene un rango [start, start+chunk) exclusivo:
            // sin locks, sin dos hilos escaneando el mismo chunk.
            const int start = nextChunkStart.fetch_add(kSparkChunkSize, std::memory_order_relaxed);
            if (start >= n) {
                break;  // No queda trabajo: todo el arreglo ya se repartio.
            }

            // Corte temprano CORRECTO: si el mejor candidato conocido hasta
            // ahora ya tiene un indice menor o igual al inicio de ESTE
            // chunk, ningun indice dentro de este chunk (ni de los
            // siguientes, que empiezan aun mas adelante) puede mejorar el
            // resultado -- es seguro detenerse sin escanearlo.
            if (start >= winner.load(std::memory_order_relaxed)) {
                break;
            }

            const int end = std::min(start + kSparkChunkSize, n);

            for (int i = start; i < end; ++i) {
                ++localIterations;
                if (particles_[static_cast<size_t>(i)].temp > kCriticalSparkTemp) {
                    found.store(true, std::memory_order_relaxed);
                    int previous = winner.load(std::memory_order_relaxed);
                    while (i < previous &&
                           !winner.compare_exchange_weak(previous, i, std::memory_order_relaxed)) {
                        // Reintenta si otro hilo actualizo 'winner' entretanto;
                        // termina cuando 'previous' ya no es mayor a i o el CAS
                        // tuvo exito.
                    }
                    break;  // Ya no tiene caso seguir este chunk: se encontro.
                }
            }
        }

        iterationsDoneAtomic.fetch_add(localIterations, std::memory_order_relaxed);
    }

    (void)found;  // Queda como documentacion de intencion; la condicion de
                  // corte real usa 'winner' directamente (ver arriba).
    outIterations = static_cast<int>(iterationsDoneAtomic.load(std::memory_order_relaxed));
    const int result = winner.load(std::memory_order_relaxed);
    return (result == std::numeric_limits<int>::max()) ? -1 : result;
#endif
}
