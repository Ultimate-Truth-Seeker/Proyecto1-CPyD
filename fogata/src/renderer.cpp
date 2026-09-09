#include "renderer.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

constexpr float kTrailDecay    = 0.55f;
constexpr float kExposure      = 1.05f;
constexpr float kFlameBoost    = 0.58f;
constexpr float kEmberBoost    = 3.60f;
constexpr float kSmokeBoost    = 0.022f;
constexpr int   kReferenceN    = 3000;
constexpr float kMaxStretch    = 1.85f;
constexpr int   kGammaLutSize  = 1024;

constexpr int   kBloomScale    = 4;
constexpr int   kBloomRadius   = 5;
constexpr float kBloomStrength = 0.34f;
constexpr int   kParticleTileSize = 32;

constexpr int   kStarCount     = 520;
constexpr float kHorizonRatio  = 0.78f;
constexpr float kVignette      = 0.74f;

constexpr float kSmokeColor[3]     = {0.145f, 0.132f, 0.128f};
constexpr float kStarColor[3]      = {0.82f, 0.88f, 1.00f};
constexpr float kFirelightColor[3] = {1.00f, 0.40f, 0.12f};
constexpr float kBlueCore[3]       = {0.30f, 0.55f, 1.00f};

constexpr float kRamp[][4] = {
    {0.00f, 0.05f, 0.005f, 0.010f},
    {0.12f, 0.35f, 0.030f, 0.010f},
    {0.30f, 0.90f, 0.140f, 0.020f},
    {0.50f, 1.00f, 0.380f, 0.050f},
    {0.70f, 1.00f, 0.640f, 0.150f},
    {0.86f, 1.00f, 0.850f, 0.420f},
    {1.00f, 1.00f, 0.920f, 0.620f},
};
constexpr int kRampSize = static_cast<int>(sizeof(kRamp) / sizeof(kRamp[0]));

const char kCharset[] = " .,:-/=%0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const uint8_t kGlyphs[][7] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x04},
    {0x00,0x00,0x00,0x00,0x00,0x04,0x08},
    {0x00,0x00,0x04,0x00,0x04,0x00,0x00},
    {0x00,0x00,0x00,0x0E,0x00,0x00,0x00},
    {0x01,0x01,0x02,0x04,0x08,0x10,0x10},
    {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00},
    {0x11,0x12,0x02,0x04,0x08,0x09,0x11},
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
    {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E},
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
    {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C},
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C},
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},
    {0x11,0x11,0x19,0x15,0x13,0x11,0x11},
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11},
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
};

const uint8_t* glyphFor(char c) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    if (c == '\0') return nullptr;
    const char* found = std::strchr(kCharset, c);
    return (found == nullptr) ? nullptr : kGlyphs[found - kCharset];
}

std::string oneDecimal(float value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.1f", static_cast<double>(value));
    return buffer;
}

float clamp01(float value) {
    return (value < 0.0f) ? 0.0f : (value > 1.0f) ? 1.0f : value;
}

float vignetteAt(int x, int y, int width, int height) {
    const float nx = static_cast<float>(x) / static_cast<float>(width  - 1) * 2.0f - 1.0f;
    const float ny = static_cast<float>(y) / static_cast<float>(height - 1) * 2.0f - 1.0f;
    const float radial = (nx * nx + ny * ny) * 0.5f;
    return std::max(0.20f, 1.0f - kVignette * radial * radial);
}

// Cuanto tarda un ciclo completo alternando entre paleta de fuego (cuerpo
// negro) y paleta arcoiris, en segundos. Sube y baja como una onda
// triangular: kRainbowCycleSeconds/2 subiendo, kRainbowCycleSeconds/2
// bajando -- asi la transicion es gradual en ambos sentidos, nunca un salto.
constexpr float kRainbowCycleSeconds = 14.0f;

// Cuantas vueltas completas de matiz (hue) da la paleta arcoiris durante el
// tiempo en que paletteMix esta por encima de cero. Un valor mayor a 1 hace
// que el arcoiris "viaje" a lo largo del espectro en vez de quedarse fijo
// en un solo tono mientras dura la mezcla.
constexpr float kRainbowHueCycles = 1.6f;

// Conversion HSV -> RGB estandar, con h en [0,1) (no en grados), s y v en
// [0,1]. Se usa para generar la paleta arcoiris de colorForTemperature():
// recorrer h de 0 a 1 pasa por todo el espectro visible (rojo -> amarillo
// -> verde -> cian -> azul -> magenta -> rojo).
void hsvToRgb(float h, float s, float v, float* outR, float* outG, float* outB) {
    const float hh = (h - std::floor(h)) * 6.0f;
    const int   i  = static_cast<int>(hh);
    const float f  = hh - static_cast<float>(i);
    const float p  = v * (1.0f - s);
    const float q  = v * (1.0f - s * f);
    const float t  = v * (1.0f - s * (1.0f - f));
    switch (i) {
        case 0:  *outR = v; *outG = t; *outB = p; break;
        case 1:  *outR = q; *outG = v; *outB = p; break;
        case 2:  *outR = p; *outG = v; *outB = t; break;
        case 3:  *outR = p; *outG = q; *outB = v; break;
        case 4:  *outR = t; *outG = p; *outB = v; break;
        default: *outR = v; *outG = p; *outB = q; break;
    }
}

}  // namespace

Renderer::~Renderer() {
    if (texture_  != nullptr) SDL_DestroyTexture(texture_);
    if (renderer_ != nullptr) SDL_DestroyRenderer(renderer_);
    if (window_   != nullptr) SDL_DestroyWindow(window_);
    texture_  = nullptr;
    renderer_ = nullptr;
    window_   = nullptr;
}

bool Renderer::init(const Config& cfg, const FireSystem& fire, std::string& error) {
    width_     = cfg.width;
    height_    = cfg.height;
    intensity_ = cfg.intensity;
    density_   = std::min(3.0f, std::max(0.10f,
                 static_cast<float>(kReferenceN) / static_cast<float>(cfg.nParticles)));

    window_ = SDL_CreateWindow("Fogata - screensaver secuencial",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width_, height_, SDL_WINDOW_SHOWN);
    if (window_ == nullptr) {
        error = std::string("no se pudo crear la ventana: ") + SDL_GetError();
        return false;
    }

    Uint32 flags = SDL_RENDERER_ACCELERATED;
    if (cfg.vsync) flags |= SDL_RENDERER_PRESENTVSYNC;
    renderer_ = SDL_CreateRenderer(window_, -1, flags);
    if (renderer_ == nullptr) {
        error = std::string("no se pudo crear el renderer: ") + SDL_GetError();
        return false;
    }

    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, width_, height_);
    if (texture_ == nullptr) {
        error = std::string("no se pudo crear la textura: ") + SDL_GetError();
        return false;
    }

    const size_t pixelCount = static_cast<size_t>(width_) * height_;
    field_.assign(pixelCount * 3, 0.0f);
    background_.assign(pixelCount * 3, 0.0f);
    firelight_.assign(pixelCount, 0.0f);

    bloomWidth_  = (width_  + kBloomScale - 1) / kBloomScale;
    bloomHeight_ = (height_ + kBloomScale - 1) / kBloomScale;
    bloom_.assign(static_cast<size_t>(bloomWidth_) * bloomHeight_ * 3, 0.0f);
    bloomRaw_.assign(bloom_.size(), 0.0f);
    bloomScratch_.assign(bloom_.size(), 0.0f);

    particleTileWidth_ = (width_ + kParticleTileSize - 1) / kParticleTileSize;
    particleTileHeight_ = (height_ + kParticleTileSize - 1) / kParticleTileSize;
    particleTiles_.resize(static_cast<size_t>(particleTileWidth_) *
                          particleTileHeight_);

    Rng rng(cfg.seed ^ 0xA5A5A5A5u);
    buildPalette();
    buildNightSky(rng);
    buildFirelight(fire);
    buildSparkSprite();
    return true;
}

void Renderer::colorForTemperature(float temp, float time, float* outR, float* outG, float* outB) const {
    const int index = std::min(255, std::max(0, static_cast<int>(temp * 255.0f)));
    const float fireR = blackbody_[index][0];
    const float fireG = blackbody_[index][1];
    const float fireB = blackbody_[index][2];

    if (paletteMix_ <= 0.0f) {
        *outR = fireR; *outG = fireG; *outB = fireB;
        return;
    }

    // El matiz del arcoiris avanza con el tiempo (para que "viaje" por el
    // espectro) y tambien varia un poco con la temperatura de la particula
    // (para que no todas las particulas del frame sean el mismo color solido
    // -- conservan algo de variacion visual entre si, como en la paleta de
    // fuego original).
    const float hue = std::fmod(time / kRainbowCycleSeconds * kRainbowHueCycles +
                                temp * 0.25f, 1.0f);
    float rainbowR, rainbowG, rainbowB;
    hsvToRgb(hue, 0.85f, 1.0f, &rainbowR, &rainbowG, &rainbowB);

    // La intensidad (brillo) sigue viniendo de la temperatura real de la
    // particula, igual que con la paleta de cuerpo negro -- solo el MATIZ
    // cambia a arcoiris. Sin esto, particulas frias (casi extintas)
    // brillarian igual de fuerte que las calientes, rompiendo la logica de
    // enfriamiento visual que ya tenia la fogata.
    const float brightness = fireR + fireG + fireB;
    rainbowR *= brightness;
    rainbowG *= brightness;
    rainbowB *= brightness;

    *outR = fireR + (rainbowR - fireR) * paletteMix_;
    *outG = fireG + (rainbowG - fireG) * paletteMix_;
    *outB = fireB + (rainbowB - fireB) * paletteMix_;
}

void Renderer::buildPalette() {
    for (int i = 0; i < 256; ++i) {
        const float t = static_cast<float>(i) / 255.0f;
        int segment = 0;
        while (segment < kRampSize - 2 && t > kRamp[segment + 1][0]) ++segment;
        const float t0 = kRamp[segment][0];
        const float t1 = kRamp[segment + 1][0];
        const float k  = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;
        for (int channel = 0; channel < 3; ++channel) {
            blackbody_[i][channel] = kRamp[segment][channel + 1] +
                (kRamp[segment + 1][channel + 1] - kRamp[segment][channel + 1]) * k;
        }
    }

    for (int i = 0; i < kGammaLutSize; ++i) {
        const float mapped = static_cast<float>(i) / (kGammaLutSize - 1);
        gammaLut_[i] = static_cast<uint8_t>(std::pow(mapped, 1.0f / 2.2f) * 255.0f + 0.5f);
    }
}

void Renderer::buildNightSky(Rng& rng) {
    const int horizon = static_cast<int>(static_cast<float>(height_) * kHorizonRatio);
    const float blend = static_cast<float>(height_) * 0.035f;

    for (int y = 0; y < height_; ++y) {
        const float skyDepth = static_cast<float>(y) / static_cast<float>(std::max(1, horizon));
        const float onGround = clamp01((static_cast<float>(y - horizon) + blend) / (2.0f * blend));
        const float depth = clamp01(static_cast<float>(y - horizon) /
                                    static_cast<float>(std::max(1, height_ - horizon)));

        const float sky[3] = {
            0.0055f + 0.0060f * skyDepth,
            0.0065f + 0.0050f * skyDepth,
            0.0210f + 0.0040f * skyDepth,
        };

        // El suelo ya no es un degradado casi plano: se oscurece un poco
        // segun la profundidad (mas lejos del horizonte = ligeramente mas
        // oscuro, simulando que la luz de la fogata/estrellas llega menos)
        // y tiene motas de "tierra" mas marcadas que antes (kSoilGrainMin/Max
        // en vez del rango casi imperceptible que tenia el fondo previo).
        const float fade = 1.0f - 0.55f * depth;

        for (int x = 0; x < width_; ++x) {
            const float dim = vignetteAt(x, y, width_, height_);

            if (onGround <= 0.0f) {
                // Todavia en el cielo: sin textura de tierra.
                float* out = &background_[(static_cast<size_t>(y) * width_ + x) * 3];
                for (int c = 0; c < 3; ++c) out[c] = sky[c] * dim;
                continue;
            }

            // Textura de tierra: tres octavas de ruido barato.
            // Grano fino aleatorio + ondas medias + ondas largas ("parcelas").
            const float fineGrain   = rng.nextFloat();
            const float medGrain    = 0.5f + 0.5f * std::sin(static_cast<float>(x) * 0.13f +
                                                             static_cast<float>(y) * 0.09f +
                                                             fineGrain * 6.2831853f);
            const float coarseGrain = 0.5f + 0.5f * std::sin(static_cast<float>(x) * 0.031f +
                                                             static_cast<float>(y) * 0.022f);
            const float grain = 0.45f + 0.25f * fineGrain
                                     + 0.18f * medGrain
                                     + 0.12f * coarseGrain;

            // Hierba esporadica: puntos verticales algo mas claros y verdosos
            // cerca del horizonte (donde la distancia los hace sutiles).
            const float grassProb = rng.nextFloat();
            const float grassBlade = (grassProb < 0.055f && depth < 0.18f) ? 1.8f : 1.0f;
            const float grassTint  = (grassProb < 0.055f && depth < 0.18f) ? 1.35f : 1.0f;

            // Tono tierra/marron oscuro con veta verdosa cerca del horizonte.
            const float soil[3] = {
                0.032f * grain * fade * grassBlade,
                0.024f * grain * fade * grassBlade * grassTint,
                0.014f * grain * fade * grassBlade,
            };
            float* out = &background_[(static_cast<size_t>(y) * width_ + x) * 3];
            for (int c = 0; c < 3; ++c) {
                out[c] = (sky[c] + (soil[c] - sky[c]) * onGround) * dim;
            }
        }
    }

    stars_.clear();
    stars_.reserve(kStarCount);
    const int starCeiling = static_cast<int>(static_cast<float>(horizon) * 0.94f);
    for (int i = 0; i < kStarCount; ++i) {
        const int x = static_cast<int>(rng.nextFloat() * static_cast<float>(width_));
        const int y = static_cast<int>(rng.nextFloat() * static_cast<float>(starCeiling));
        if (x >= width_ || y >= height_) continue;
        const float far = 1.0f - static_cast<float>(y) / static_cast<float>(starCeiling);
        // Las estrellas mas cerca del horizonte (far~0) se mueven mas lento;
        // las del cenit (far~1) se mueven mas rapido -- efecto paralaje.
        const float speed = 0.35f + 0.65f * far;
        stars_.push_back({x, y,
                          rng.range(0.10f, 0.85f) * (0.45f + 0.55f * far) *
                              vignetteAt(x, y, width_, height_),
                          rng.range(0.0f, 6.2831853f),
                          speed});
    }

    // Generar arboles procedurales: siluetas de pino a distintas distancias
    // (simuladas con escala) distribuidos a lo largo del horizonte,
    // evitando el centro donde esta la fogata.
    trees_.clear();
    const int treeCount = 14;
    trees_.reserve(treeCount);
    for (int i = 0; i < treeCount; ++i) {
        Tree t;
        // Distribuir en dos grupos: izquierda y derecha de la fogata.
        // Fraccion en [0,1] del ancho de pantalla.
        if (i < treeCount / 2) {
            t.x = rng.range(0.02f, 0.34f);
        } else {
            t.x = rng.range(0.66f, 0.98f);
        }
        t.scale  = rng.range(0.45f, 1.0f);   // escala: arboles mas pequeños = mas lejos
        t.layers = 3 + static_cast<int>(rng.nextFloat() * 3.0f); // 3-5 capas
        t.lean   = rng.range(-0.04f, 0.04f); // ligera inclinacion
        trees_.push_back(t);
    }
}

void Renderer::buildFirelight(const FireSystem& fire) {
    const int   horizon   = static_cast<int>(static_cast<float>(height_) * kHorizonRatio);
    const float centerX   = fire.hearthX();
    const float centerY   = fire.hearthY();
    const float blend     = static_cast<float>(height_) * 0.035f;
    const float poolRange = fire.hearthHalfWidth() * 2.2f;
    const float airRange  = fire.hearthHalfWidth() * 2.8f;

    for (int y = 0; y < height_; ++y) {
        const float dy = static_cast<float>(y) - centerY;
        const float onGround =
            clamp01((static_cast<float>(y - horizon) + blend) / (2.0f * blend));

        for (int x = 0; x < width_; ++x) {
            const float dx = static_cast<float>(x) - centerX;

            const float pool = std::sqrt(dx * dx + (dy * 2.6f) * (dy * 2.6f)) / poolRange;
            const float ground = 0.34f / (1.0f + pool * pool * pool);

            const float halo = std::sqrt(dx * dx + (dy * 1.15f) * (dy * 1.15f)) / airRange;
            const float air = 0.13f / (1.0f + 2.4f * halo * halo);

            firelight_[static_cast<size_t>(y) * width_ + x] =
                (air + (ground - air) * onGround) * vignetteAt(x, y, width_, height_);
        }
    }
}

void Renderer::accumulateStars(float time) {
    // Velocidad angular de la boveda estelar en pixeles/segundo.
    // Con kStarDriftSpeed = 4.0 y una pantalla de 1280px, una vuelta
    // completa (width_ pixeles) tardaria ~320 segundos: imperceptible
    // frame a frame pero apreciable en el transcurso de la sesion.
    constexpr float kStarDriftSpeed = 4.0f;

    for (const Star& star : stars_) {
        // Offset horizontal que avanza con el tiempo; se envuelve con modulo
        // para que las estrellas que salen por la derecha reaparezcan por la
        // izquierda sin salto visible.
        const float drift = kStarDriftSpeed * star.speed * time;
        const int px = (star.x + static_cast<int>(drift)) % width_;
        const int px_wrapped = (px < 0) ? px + width_ : px;
        const int py = star.y;
        if (py < 0 || py >= height_ || px_wrapped < 0 || px_wrapped >= width_) continue;

        const float twinkle = star.brightness *
                              (0.55f + 0.45f * std::sin(time * 1.7f + star.phase));
        float* out = &field_[(static_cast<size_t>(py) * width_ + px_wrapped) * 3];
        out[0] += kStarColor[0] * twinkle;
        out[1] += kStarColor[1] * twinkle;
        out[2] += kStarColor[2] * twinkle;
    }
}

void Renderer::accumulateParticles(const FireSystem& fire) {
    const float exposure  = intensity_ * density_;
    const float hearthY   = fire.hearthY();
    const float blueRange = 70.0f * fire.scale();
    const float time      = fire.elapsed();

    for (std::vector<int>& tile : particleTiles_)
        tile.clear();

    const std::vector<Particle>& particles = fire.particles();
    for (int pi = 0; pi < static_cast<int>(particles.size()); ++pi) {
        const Particle& p = particles[pi];
        const float stretch = std::min(kMaxStretch, 1.0f + std::fabs(p.vy) * 0.006f);
        const int x0 = std::max(0, static_cast<int>(p.x - p.radius));
        const int x1 = std::min(width_ - 1, static_cast<int>(p.x + p.radius));
        const int y0 = std::max(0, static_cast<int>(p.y - p.radius * stretch));
        const int y1 = std::min(height_ - 1, static_cast<int>(p.y + p.radius * stretch));
        if (x0 > x1 || y0 > y1)
            continue;

        const int tileX0 = x0 / kParticleTileSize;
        const int tileX1 = x1 / kParticleTileSize;
        const int tileY0 = y0 / kParticleTileSize;
        const int tileY1 = y1 / kParticleTileSize;
        for (int tileY = tileY0; tileY <= tileY1; ++tileY) {
            for (int tileX = tileX0; tileX <= tileX1; ++tileX) {
                particleTiles_[static_cast<size_t>(tileY) * particleTileWidth_ + tileX]
                    .push_back(pi);
            }
        }
    }

    #pragma omp parallel for schedule(dynamic)
    for (int tileIndex = 0;
         tileIndex < static_cast<int>(particleTiles_.size()); ++tileIndex) {
        const int tileX = tileIndex % particleTileWidth_;
        const int tileY = tileIndex / particleTileWidth_;
        const int tileX0 = tileX * kParticleTileSize;
        const int tileY0 = tileY * kParticleTileSize;
        const int tileX1 = std::min(width_ - 1, tileX0 + kParticleTileSize - 1);
        const int tileY1 = std::min(height_ - 1, tileY0 + kParticleTileSize - 1);

        for (int pi : particleTiles_[tileIndex]) {
        const Particle& p = particles[pi];
        float colorR, colorG, colorB;

        if (p.kind == ParticleKind::Smoke) {
            const float emission = (0.35f + p.temp) * kSmokeBoost * exposure;
            colorR = kSmokeColor[0] * p.tintR * emission;
            colorG = kSmokeColor[1] * p.tintG * emission;
            colorB = kSmokeColor[2] * p.tintB * emission;
        } else {
            const float boost = (p.kind == ParticleKind::Ember) ? kEmberBoost : kFlameBoost;
            const float emission = p.temp * p.temp * boost * exposure;
            if (emission <= 0.008f) continue;

            float baseR, baseG, baseB;
            colorForTemperature(p.temp, time, &baseR, &baseG, &baseB);
            colorR = baseR * p.tintR * emission;
            colorG = baseG * p.tintG * emission;
            colorB = baseB * p.tintB * emission;

            if (p.kind == ParticleKind::Flame) {
                const float blue = clamp01((p.temp - 0.88f) * 8.0f) *
                                   clamp01(1.0f - (hearthY - p.y) / blueRange) * 0.80f;
                if (blue > 0.0f) {
                    colorR += (kBlueCore[0] * emission - colorR) * blue;
                    colorG += (kBlueCore[1] * emission - colorG) * blue;
                    colorB += (kBlueCore[2] * emission - colorB) * blue;
                }
            }
        }

        const float stretch = std::min(kMaxStretch, 1.0f + std::fabs(p.vy) * 0.006f);
        const float radiusX = p.radius;
        const float radiusY = p.radius * stretch;
        const float invX    = 1.0f / (radiusX * radiusX);
        const float invY    = 1.0f / (radiusY * radiusY);

        const int x0 = std::max(tileX0, static_cast<int>(p.x - radiusX));
        const int x1 = std::min(tileX1, static_cast<int>(p.x + radiusX));
        const int y0 = std::max(tileY0, static_cast<int>(p.y - radiusY));
        const int y1 = std::min(tileY1, static_cast<int>(p.y + radiusY));
        if (x0 > x1 || y0 > y1)
            continue;

        for (int y = y0; y <= y1; ++y) {
            const float dy   = static_cast<float>(y) - p.y;
            const float dySq = dy * dy * invY;
            if (dySq >= 1.0f) continue;
            float* row = &field_[(static_cast<size_t>(y) * width_ + x0) * 3];
            for (int x = x0; x <= x1; ++x, row += 3) {
                const float dx = static_cast<float>(x) - p.x;
                const float d2 = dx * dx * invX + dySq;
                if (d2 >= 1.0f) continue;
                const float falloff = 1.0f - d2;
                const float weight  = falloff * falloff;
                row[0] += colorR * weight;
                row[1] += colorG * weight;
                row[2] += colorB * weight;
            }
        }
        }
    }
}

void Renderer::buildBloomRaw() {
    #pragma omp parallel for schedule(static)
    for (int blockY = 0; blockY < bloomHeight_; ++blockY) {
        float* raw = &bloomRaw_[static_cast<size_t>(blockY) * bloomWidth_ * 3];
        std::fill(raw, raw + static_cast<size_t>(bloomWidth_) * 3, 0.0f);

        const int y0 = blockY * kBloomScale;
        const int y1 = std::min(height_, y0 + kBloomScale);
        for (int y = y0; y < y1; ++y) {
            const float* src = &field_[static_cast<size_t>(y) * width_ * 3];
            for (int x = 0; x < width_; ++x) {
                float* dst = &raw[(x / kBloomScale) * 3];
                dst[0] += src[x * 3 + 0];
                dst[1] += src[x * 3 + 1];
                dst[2] += src[x * 3 + 2];
            }
        }
    }
}

void Renderer::composite(float glow) {
    #pragma omp parallel for
    for (int y = 0; y < height_; ++y) {
        const int blockY = y / kBloomScale;
        const float* halo = &bloom_[static_cast<size_t>(blockY) * bloomWidth_ * 3];
        const float* bg   = &background_[static_cast<size_t>(y) * width_ * 3];
        const float* lit  = &firelight_[static_cast<size_t>(y) * width_];
        float*       src  = &field_[static_cast<size_t>(y) * width_ * 3];
        uint32_t*    dst  = &pixels_[static_cast<size_t>(y) * pitch_];

        int blockX = 0;
        int blockStep = 0;

        for (int x = 0; x < width_; ++x, src += 3, bg += 3) {
            const float* glowRow = halo + blockX * 3;
            const float  warm    = lit[x] * glow;

            uint32_t argb = 0xFF000000u;
            for (int c = 0; c < 3; ++c) {
                const float light = src[c];
                const float value = light * kExposure + glowRow[c] * kBloomStrength +
                                    bg[c] + warm * kFirelightColor[c];
                const float mapped = value / (1.0f + value);
                argb |= static_cast<uint32_t>(
                            gammaLut_[static_cast<int>(mapped * (kGammaLutSize - 1))])
                        << (16 - 8 * c);
                src[c] = light * kTrailDecay;
            }
            dst[x] = argb;

            if (++blockStep == kBloomScale) {
                blockStep = 0;
                ++blockX;
            }
        }
    }
}

void Renderer::blurBloom() {
    const float norm = 1.0f / (static_cast<float>(2 * kBloomRadius + 1) *
                               static_cast<float>(kBloomScale * kBloomScale));

    #pragma omp parallel for
    for (int y = 0; y < bloomHeight_; ++y) {
        const float* src = &bloomRaw_[static_cast<size_t>(y) * bloomWidth_ * 3];
        float*       dst = &bloomScratch_[static_cast<size_t>(y) * bloomWidth_ * 3];
        float sum[3] = {0.0f, 0.0f, 0.0f};
        for (int x = -kBloomRadius; x <= kBloomRadius; ++x) {
            const int sample = std::min(bloomWidth_ - 1, std::max(0, x));
            for (int c = 0; c < 3; ++c) sum[c] += src[sample * 3 + c];
        }
        for (int x = 0; x < bloomWidth_; ++x) {
            const int add = std::min(bloomWidth_ - 1, x + kBloomRadius + 1);
            const int sub = std::max(0, x - kBloomRadius);
            for (int c = 0; c < 3; ++c) {
                dst[x * 3 + c] = sum[c] * norm;
                sum[c] += src[add * 3 + c] - src[sub * 3 + c];
            }
        }
    }

    const int stride = bloomWidth_ * 3;
    const float vertical = 1.0f / static_cast<float>(2 * kBloomRadius + 1);

    #pragma omp parallel for
    for (int x = 0; x < bloomWidth_; ++x) {
        const float* src = &bloomScratch_[static_cast<size_t>(x) * 3];
        float*       dst = &bloom_[static_cast<size_t>(x) * 3];
        float sum[3] = {0.0f, 0.0f, 0.0f};
        for (int y = -kBloomRadius; y <= kBloomRadius; ++y) {
            const int sample = std::min(bloomHeight_ - 1, std::max(0, y));
            for (int c = 0; c < 3; ++c) sum[c] += src[sample * stride + c];
        }
        for (int y = 0; y < bloomHeight_; ++y) {
            const int add = std::min(bloomHeight_ - 1, y + kBloomRadius + 1);
            const int sub = std::max(0, y - kBloomRadius);
            for (int c = 0; c < 3; ++c) {
                dst[y * stride + c] = sum[c] * vertical;
                sum[c] += src[add * stride + c] - src[sub * stride + c];
            }
        }
    }
}

void Renderer::drawGround(const FireSystem& fire) {
    // Banda de suelo iluminada cerca de la fogata: un gradiente horizontal
    // color tierra-calida que se mezcla sobre el background ya compuesto.
    // Solo afecta la franja visible del suelo (desde el horizonte hacia abajo)
    // y se desvanece hacia los bordes de la pantalla.
    const int   horizon  = static_cast<int>(static_cast<float>(height_) * kHorizonRatio);
    const float centerX  = fire.hearthX();
    const float halfW    = fire.hearthHalfWidth();
    const float glow     = fire.flicker();
    const float poolR    = halfW * 5.5f;  // radio del charco de luz en el suelo

    for (int y = horizon; y < height_; ++y) {
        // Cuanto mas lejos del horizonte mas tenue (suelo lejano esta en sombra).
        const float depth = static_cast<float>(y - horizon) /
                            static_cast<float>(std::max(1, height_ - horizon));
        const float depthFade = 1.0f - 0.80f * depth;

        for (int x = 0; x < width_; ++x) {
            const float dx  = static_cast<float>(x) - centerX;
            // La luz se aplana horizontalmente (perspectiva del suelo).
            const float dist = std::fabs(dx) / poolR;
            if (dist >= 1.0f) continue;

            const float radial = (1.0f - dist * dist) * depthFade * glow * 0.28f;
            if (radial <= 0.002f) continue;

            const size_t offset = static_cast<size_t>(y) * pitch_ + x;
            const uint32_t back = pixels_[offset];
            const int backR = static_cast<int>((back >> 16) & 0xFF);
            const int backG = static_cast<int>((back >>  8) & 0xFF);
            const int backB = static_cast<int>( back        & 0xFF);

            // Tono naranja-calido de la luz de la fogata sobre la tierra.
            const int mr = std::min(255, backR + static_cast<int>(radial * 180.0f));
            const int mg = std::min(255, backG + static_cast<int>(radial *  72.0f));
            const int mb = std::min(255, backB + static_cast<int>(radial *  18.0f));
            pixels_[offset] = 0xFF000000u | (static_cast<uint32_t>(mr) << 16) |
                              (static_cast<uint32_t>(mg) << 8) | static_cast<uint32_t>(mb);
        }
    }
}

void Renderer::drawTrees(const FireSystem& fire) {
    // Siluetas de pino dibujadas sobre pixels_ como geometria procedural.
    // Cada arbol es una serie de triangulos apilados ("capas") que se
    // estrechan hacia arriba, mas un tronco rectangular en la base.
    // El color base es casi negro (silueta nocturna); el borde inferior de
    // cada capa recibe un sutil tinte naranja de la luz de la fogata,
    // proporcional a la distancia horizontal al centro.

    const int   horizon  = static_cast<int>(static_cast<float>(height_) * kHorizonRatio);
    const float centerX  = fire.hearthX();
    const float halfW    = fire.hearthHalfWidth();
    const float glow     = fire.flicker();
    const float baseH    = static_cast<float>(height_);

    // Altura y ancho maximos de un arbol de escala=1 (en pixeles relativos
    // a la altura de pantalla).
    constexpr float kTreeMaxHeight = 0.32f;  // fraccion de height_
    constexpr float kTreeMaxWidth  = 0.055f; // fraccion de height_ (no width_)
    constexpr float kTrunkFrac     = 0.12f;  // fraccion de la altura del arbol

    for (const Tree& tree : trees_) {
        const float treeH  = baseH * kTreeMaxHeight * tree.scale;
        const float treeW  = baseH * kTreeMaxWidth  * tree.scale;
        const float baseX  = tree.x * static_cast<float>(width_);
        // La base del arbol esta en el horizonte; el tronco baja un poco mas.
        const float baseY  = static_cast<float>(horizon);
        const float topY   = baseY - treeH;
        const float trunkH = treeH * kTrunkFrac;
        const float trunkW = treeW * 0.12f;

        // Distancia horizontal al fuego para la iluminacion ambiental.
        const float distToFire = std::fabs(baseX - centerX);
        // Cuanta luz de la fogata llega a este arbol (0 si muy lejos).
        const float fireProximity = clamp01(1.0f - distToFire / (halfW * 9.0f));
        const float edgeGlow = glow * fireProximity;

        // --- Tronco ---
        {
            const int tx0 = std::max(0,          static_cast<int>(baseX - trunkW));
            const int tx1 = std::min(width_ - 1, static_cast<int>(baseX + trunkW));
            const int ty0 = std::max(0,          static_cast<int>(baseY - trunkH));
            const int ty1 = std::min(height_ - 1,static_cast<int>(baseY + 4.0f));
            for (int py = ty0; py <= ty1; ++py) {
                for (int px = tx0; px <= tx1; ++px) {
                    const size_t off = static_cast<size_t>(py) * pitch_ + px;
                    // Tronco oscuro con tinte muy leve de fuego en la cara visible.
                    const float edge = (static_cast<float>(px) - baseX) / trunkW; // [-1,1]
                    const float lit  = edgeGlow * clamp01(1.0f - std::fabs(edge)) * 0.18f;
                    const int r = std::min(255, 14 + static_cast<int>(lit * 140.0f));
                    const int g = std::min(255,  9 + static_cast<int>(lit *  55.0f));
                    const int b = std::min(255,  7 + static_cast<int>(lit *  14.0f));
                    pixels_[off] = 0xFF000000u | (static_cast<uint32_t>(r) << 16) |
                                   (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
                }
            }
        }

        // --- Capas del pino (de abajo hacia arriba) ---
        const int   numLayers  = tree.layers;
        const float layerStep  = (treeH - trunkH) / static_cast<float>(numLayers);

        for (int layer = 0; layer < numLayers; ++layer) {
            // Cada capa es un triangulo con base ancha abajo y punta arriba.
            // Las capas de abajo son mas anchas; las de arriba mas estrechas.
            const float layerFrac  = static_cast<float>(layer) / static_cast<float>(numLayers);
            const float nextFrac   = static_cast<float>(layer + 1) / static_cast<float>(numLayers);
            // La base de la capa esta un poco por encima del tronco.
            const float layerBaseY = baseY - trunkH - layerStep * static_cast<float>(layer);
            const float layerTopY  = layerBaseY - layerStep * 1.15f; // ligero solapado
            const float layerBaseW = treeW * (1.0f - 0.25f * layerFrac);  // se estrecha
            const float layerTopW  = treeW * (0.08f);                      // punta fina

            const int py0 = std::max(0,          static_cast<int>(layerTopY));
            const int py1 = std::min(height_ - 1,static_cast<int>(layerBaseY));

            for (int py = py0; py <= py1; ++py) {
                // Interpolacion lineal del ancho entre tope y base de esta capa.
                const float t = (static_cast<float>(py) - layerTopY) /
                                 std::max(1.0f, layerBaseY - layerTopY);
                const float halfCap = layerTopW + (layerBaseW - layerTopW) * t;
                // Inclinacion suave del arbol.
                const float lean = tree.lean * static_cast<float>(height_) *
                                   (1.0f - layerFrac - t * (1.0f - layerFrac) * 0.5f);
                const int px0 = std::max(0,          static_cast<int>(baseX - halfCap + lean));
                const int px1 = std::min(width_ - 1, static_cast<int>(baseX + halfCap + lean));

                for (int px = px0; px <= px1; ++px) {
                    // La base de cada capa (t cercano a 1) recibe mas luz de fogata;
                    // la punta (t cercano a 0) casi no recibe.
                    const float warmth = edgeGlow * t * clamp01(nextFrac) * 0.32f;
                    // Borde lateral de la capa: algo mas iluminado para dar
                    // volumen a las ramas (backlight muy sutil).
                    const float nx = (static_cast<float>(px) - baseX) / std::max(1.0f, halfCap);
                    const float sideLight = edgeGlow * (1.0f - std::fabs(nx)) * 0.08f * t;

                    const int r = std::min(255,  8 + static_cast<int>((warmth + sideLight) * 220.0f));
                    const int g = std::min(255, 14 + static_cast<int>((warmth * 0.42f + sideLight * 1.2f) * 220.0f));
                    const int b = std::min(255,  8 + static_cast<int>((warmth * 0.10f + sideLight * 0.5f) * 220.0f));
                    const size_t off = static_cast<size_t>(py) * pitch_ + px;
                    pixels_[off] = 0xFF000000u | (static_cast<uint32_t>(r) << 16) |
                                   (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
                }
            }
        }

        // --- Sombra proyectada en el suelo ---
        // La sombra se dibuja como una elipse muy aplanada debajo del tronco.
        // Direccion hacia el fuego: la sombra apunta al lado contrario.
        {
            const float shadowDir  = (baseX < centerX) ? -1.0f : 1.0f; // lado opuesto al fuego
            const float shadowLen  = treeW * 2.8f * (0.4f + 0.6f * fireProximity);
            const float shadowW    = trunkW * 1.8f;
            const float shadowY0   = baseY;
            const float shadowY1   = std::min(static_cast<float>(height_ - 1), baseY + shadowW);

            const int sx0 = std::max(0,          static_cast<int>(baseX));
            const int sx1 = std::min(width_ - 1, static_cast<int>(baseX + shadowDir * shadowLen));
            const int lx0 = std::min(sx0, sx1);
            const int lx1 = std::max(sx0, sx1);
            const int sy0 = std::max(0,          static_cast<int>(shadowY0 - shadowW * 0.3f));
            const int sy1 = std::min(height_ - 1,static_cast<int>(shadowY1));

            for (int py = sy0; py <= sy1; ++py) {
                // La sombra se desvanece hacia la punta.
                const float tShadow = static_cast<float>(py - sy0) /
                                      std::max(1.0f, static_cast<float>(sy1 - sy0));
                for (int px = lx0; px <= lx1; ++px) {
                    const float tLen = std::fabs(static_cast<float>(px) - baseX) /
                                       std::max(1.0f, std::fabs(shadowDir * shadowLen));
                    const float alpha = (1.0f - tLen) * (1.0f - tShadow) *
                                        fireProximity * 0.55f;
                    if (alpha <= 0.01f) continue;
                    const size_t off = static_cast<size_t>(py) * pitch_ + px;
                    const uint32_t back = pixels_[off];
                    const int backR = static_cast<int>((back >> 16) & 0xFF);
                    const int backG = static_cast<int>((back >>  8) & 0xFF);
                    const int backB = static_cast<int>( back        & 0xFF);
                    // La sombra oscurece el suelo.
                    const int mr = static_cast<int>(backR * (1.0f - alpha * 0.7f));
                    const int mg = static_cast<int>(backG * (1.0f - alpha * 0.7f));
                    const int mb = static_cast<int>(backB * (1.0f - alpha * 0.7f));
                    pixels_[off] = 0xFF000000u | (static_cast<uint32_t>(mr) << 16) |
                                   (static_cast<uint32_t>(mg) << 8) | static_cast<uint32_t>(mb);
                }
            }
        }
    }
}

void Renderer::drawStones(const FireSystem& fire) {
    const float halfWidth = fire.hearthHalfWidth();
    const float centerX   = fire.hearthX();
    const float centerY   = fire.hearthY() + halfWidth * 0.70f;
    const float ringX     = halfWidth * 2.7f;
    const float ringY     = halfWidth * 0.42f;
    const float glow      = fire.flicker();

    for (int i = 0; i < 7; ++i) {
        const float angle = 3.1415927f * (0.05f + 0.90f * static_cast<float>(i) / 6.0f);
        const float stoneX = centerX + std::cos(angle) * ringX;
        const float stoneY = centerY + std::sin(angle) * ringY;
        const float stoneW = halfWidth * (0.30f + 0.13f * std::sin(angle * 5.3f + 1.1f));
        const float stoneH = stoneW * 0.72f;
        const float toFire = clamp01(1.0f - std::fabs(stoneX - centerX) / (ringX * 1.15f));

        const int x0 = std::max(0,           static_cast<int>(stoneX - stoneW));
        const int x1 = std::min(width_  - 1, static_cast<int>(stoneX + stoneW));
        const int y0 = std::max(0,           static_cast<int>(stoneY - stoneH));
        const int y1 = std::min(height_ - 1, static_cast<int>(stoneY + stoneH));

        for (int y = y0; y <= y1; ++y) {
            const float ny = (static_cast<float>(y) - stoneY) / stoneH;
            for (int x = x0; x <= x1; ++x) {
                const float nx = (static_cast<float>(x) - stoneX) / stoneW;
                const float d2 = nx * nx + ny * ny;
                if (d2 >= 1.0f) continue;

                if (ny > 0.45f) continue;

                const float dome  = std::sqrt(1.0f - d2);
                const float shade = 0.28f + 0.72f * dome;
                const float heat  = glow * toFire * clamp01(0.34f - ny * 0.66f) * dome;

                const int r = std::min(255, static_cast<int>(38.0f * shade + heat * 235.0f));
                const int g = std::min(255, static_cast<int>(31.0f * shade + heat * 112.0f));
                const int b = std::min(255, static_cast<int>(27.0f * shade + heat *  38.0f));

                const float alpha = std::min(1.0f, dome * 5.0f);
                const size_t offset = static_cast<size_t>(y) * pitch_ + x;
                const uint32_t back = pixels_[offset];
                const int mr = static_cast<int>(((back >> 16) & 0xFF) + (r - static_cast<int>((back >> 16) & 0xFF)) * alpha);
                const int mg = static_cast<int>(((back >>  8) & 0xFF) + (g - static_cast<int>((back >>  8) & 0xFF)) * alpha);
                const int mb = static_cast<int>(( back        & 0xFF) + (b - static_cast<int>( back        & 0xFF)) * alpha);
                pixels_[offset] = 0xFF000000u | (static_cast<uint32_t>(mr) << 16) |
                                  (static_cast<uint32_t>(mg) << 8) | static_cast<uint32_t>(mb);
            }
        }
    }
}

void Renderer::drawLogs(const FireSystem& fire) {
    const float halfWidth = fire.hearthHalfWidth();
    const float centerX   = fire.hearthX();
    const float centerY   = fire.hearthY() + halfWidth * 0.45f;
    const float span      = halfWidth * 2.1f;
    const float thick     = halfWidth * 0.30f;
    const float glow      = fire.flicker();

    struct Segment { float x0, y0, x1, y1; };
    const Segment logs[3] = {
        {centerX - span,         centerY + thick * 0.6f, centerX + span * 0.85f, centerY - thick * 0.5f},
        {centerX - span * 0.80f, centerY - thick * 0.7f, centerX + span,         centerY + thick * 0.5f},
        {centerX - span * 0.35f, centerY + thick * 1.1f, centerX + span * 0.40f, centerY + thick * 1.3f},
    };

    for (const Segment& seg : logs) {
        const float dx    = seg.x1 - seg.x0;
        const float dy    = seg.y1 - seg.y0;
        const float lenSq = std::max(1e-3f, dx * dx + dy * dy);

        const int x0 = std::max(0,           static_cast<int>(std::min(seg.x0, seg.x1) - thick));
        const int x1 = std::min(width_  - 1, static_cast<int>(std::max(seg.x0, seg.x1) + thick));
        const int y0 = std::max(0,           static_cast<int>(std::min(seg.y0, seg.y1) - thick));
        const int y1 = std::min(height_ - 1, static_cast<int>(std::max(seg.y0, seg.y1) + thick));

        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                const float px = static_cast<float>(x) - seg.x0;
                const float py = static_cast<float>(y) - seg.y0;
                const float t  = clamp01((px * dx + py * dy) / lenSq);
                const float ox = px - dx * t;
                const float oy = py - dy * t;
                const float dist = std::sqrt(ox * ox + oy * oy);
                if (dist > thick) continue;

                const float toCenter = std::fabs(static_cast<float>(x) - centerX) / span;
                const float heat  = glow * std::max(0.0f, 1.0f - toCenter) *
                                    (1.0f - dist / thick) * 0.95f;
                const float shade = 0.32f + 0.68f * (1.0f - dist / thick);

                const int r = std::min(255, static_cast<int>(28.0f * shade + heat * 240.0f));
                const int g = std::min(255, static_cast<int>(17.0f * shade + heat * 108.0f));
                const int b = std::min(255, static_cast<int>(13.0f * shade + heat *  30.0f));

                const float alpha = std::min(1.0f, (thick - dist) / (thick * 0.2f + 1e-3f));
                const size_t offset = static_cast<size_t>(y) * pitch_ + x;
                const uint32_t back = pixels_[offset];
                const int mr = static_cast<int>(((back >> 16) & 0xFF) + (r - static_cast<int>((back >> 16) & 0xFF)) * alpha);
                const int mg = static_cast<int>(((back >>  8) & 0xFF) + (g - static_cast<int>((back >>  8) & 0xFF)) * alpha);
                const int mb = static_cast<int>(( back        & 0xFF) + (b - static_cast<int>( back        & 0xFF)) * alpha);
                pixels_[offset] = 0xFF000000u | (static_cast<uint32_t>(mr) << 16) |
                                  (static_cast<uint32_t>(mg) << 8) | static_cast<uint32_t>(mb);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// buildSparkSprite -- se llama UNA vez en init().
//
// Rellena sparkSprite_[64x64] con valores de alpha en [0,1] para un disco
// de radio normalizado r en [0,1]:
//
//   r < 0.15          nucleo:  Gaussiana muy apretada (brillo central blanco)
//   0.15 <= r < 0.70  halo:    decaimiento exponencial suave
//   0.70 <= r < 0.85  anillo:  campana estrecha multiplicada por ruido angular
//                              (sumas de sin/cos de frecuencias 5,7,11,13)
//                              que da el borde "irregular" y organico
//   0.85 <= r <= 1.0  fade:    caida cuadratica a cero -- transparencia total
//
// El sprite se estampa en drawSparkHighlight() escalando el radio al tamaño
// real de la particula + offset de pulso, sin recalcular la tabla.
// -----------------------------------------------------------------------------
void Renderer::buildSparkSprite() {
    constexpr int   S    = kSparkSpriteSize;         // 64
    constexpr float half = static_cast<float>(S) * 0.5f;

    // Amplitudes de las frecuencias angulares del ruido del anillo.
    // Frecuencias impares no armonicas dan aspecto organico sin simetria obvia.
    constexpr float kNoiseAmp[4]  = { 0.055f, 0.040f, 0.028f, 0.018f };
    constexpr float kNoiseFreq[4] = { 5.0f,   7.0f,  11.0f,  13.0f  };
    constexpr float kNoisePhase[4]= { 0.0f,   1.1f,   2.3f,   0.7f  };

    for (int sy = 0; sy < S; ++sy) {
        for (int sx = 0; sx < S; ++sx) {
            const float fx = static_cast<float>(sx) - half + 0.5f;
            const float fy = static_cast<float>(sy) - half + 0.5f;
            const float r  = std::sqrt(fx * fx + fy * fy) / half; // [0, ~1.41]
            const float angle = std::atan2(fy, fx);               // [-pi, pi]

            float alpha = 0.0f;

            if (r < 0.15f) {
                // Nucleo: Gaussiana con sigma=0.07 (muy apretada)
                const float t = r / 0.15f;               // [0,1]
                alpha = std::exp(-t * t * 5.0f);          // 1 en centro, ~0 en borde

            } else if (r < 0.70f) {
                // Halo: decaimiento exponencial continuo desde el nucleo.
                // La curva empalma con el nucleo en r=0.15 (alpha~0.53)
                // y cae a ~0.04 en r=0.70 para fusionarse suavemente
                // con el anillo.
                const float t = (r - 0.15f) / (0.70f - 0.15f); // [0,1]
                alpha = 0.53f * std::exp(-t * t * 3.8f);

            } else if (r < 0.85f) {
                // Anillo irregular: campana estrecha centrada en r=0.775
                // modulada angularmente por ruido de baja frecuencia.
                const float center = 0.775f;
                const float width  = 0.065f;
                const float t = (r - center) / width;         // [-inf, inf]
                const float bell = std::exp(-t * t * 2.5f);   // pico en r=center

                // Modulacion angular: suma de senos de frecuencias no armonicas.
                // Da al anillo un aspecto de "llama" o "corona" irregular.
                float noiseScale = 1.0f;
                for (int k = 0; k < 4; ++k) {
                    noiseScale += kNoiseAmp[k] *
                        std::sin(kNoiseFreq[k] * angle + kNoisePhase[k]);
                }
                alpha = bell * noiseScale * 0.92f;

            } else if (r <= 1.0f) {
                // Fade cuadratico: de 0 en r=0.85 a 0 en r=1.0
                // (en realidad hay un pixel de transicion minimo).
                const float t = (r - 0.85f) / (1.0f - 0.85f);
                alpha = (1.0f - t) * (1.0f - t) * 0.06f;
            }
            // r > 1.0 queda alpha=0 (esquinas del cuadrado 64x64)

            sparkSprite_[static_cast<size_t>(sy) * S + sx] = clamp01(alpha);
        }
    }
}

// -----------------------------------------------------------------------------
// drawSparkHighlight -- se llama cada frame.
//
// Estampa el sprite precalculado centrado en la particula critica.
// El radio del sprite pulsa lentamente con una onda seno de ~1.1 Hz;
// la intensidad del anillo tambien pulsa (en contrafase suave) para dar
// vida sin que el nucleo parpadee.
//
// Color del sprite:
//   nucleo (alpha alto, r<0.15)  -> blanco puro / azul palido
//   halo   (0.15-0.70)          -> blanco calido que se desvanece
//   anillo (0.70-0.85)          -> blanco azulado brillante en el pico
//
// La mezcla con el fondo es alpha-compositing additive:
//   out = back + sprite_color * alpha
// (no sustituye el pixel -- suma luz encima, como hace el resto del renderer)
// -----------------------------------------------------------------------------
void Renderer::drawSparkHighlight(const FireSystem& fire, int sparkIndex) {
    if (sparkIndex < 0 || sparkIndex >= static_cast<int>(fire.particles().size())) return;

    const Particle& p   = fire.particles()[sparkIndex];
    const float     t   = fire.elapsed();

    // Pulso lento: onda seno a ~1.1 Hz en [0,1].
    // Se usa para modular el radio externo (+/-12%) y la intensidad del anillo.
    const float pulse    = 0.5f + 0.5f * std::sin(t * 6.9115f);  // 6.9115 ~ 2*pi*1.1
    const float pulseFast= 0.5f + 0.5f * std::sin(t * 9.4248f);  // ~1.5 Hz, para el nucleo

    // Radio del sprite en pixels de pantalla.
    // Base: al menos 22px o 5x el radio de la particula (para ser visible
    // incluso con particulas pequenas). El pulso anade +/-12% al radio.
    const float baseRadius = std::max(22.0f, p.radius * 5.0f);
    const float spriteR    = baseRadius * (1.0f + 0.12f * (pulse - 0.5f) * 2.0f);

    // Intensidades de cada capa, moduladas por el pulso.
    // El nucleo brilla mas cuando el anillo esta en su minimo (contrafase)
    // para que el efecto general tenga dinamismo sin saturarse todo.
    const float coreIntensity = 0.70f + 0.30f * pulseFast;          // [0.70, 1.0]
    const float haloIntensity = 0.30f + 0.15f * (1.0f - pulse);     // [0.30, 0.45]
    const float ringIntensity = 0.55f + 0.45f * pulse;              // [0.55, 1.0]

    // Bounding box del sprite en pantalla, con clipping.
    const int cx = static_cast<int>(std::round(p.x));
    const int cy = static_cast<int>(std::round(p.y));
    const int halfS = static_cast<int>(std::ceil(spriteR)) + 1;
    const int x0 = std::max(0,          cx - halfS);
    const int x1 = std::min(width_  - 1, cx + halfS);
    const int y0 = std::max(0,          cy - halfS);
    const int y1 = std::min(height_ - 1, cy + halfS);
    if (x0 > x1 || y0 > y1) return;

    constexpr int   S    = kSparkSpriteSize;
    constexpr float half = static_cast<float>(S) * 0.5f;

    for (int y = y0; y <= y1; ++y) {
        const float fy = static_cast<float>(y - cy);
        for (int x = x0; x <= x1; ++x) {
            const float fx = static_cast<float>(x - cx);

            // Coordenada normalizada en el sprite [0, S-1] via radio de pantalla.
            // spriteR corresponde a half celdas del sprite.
            const float snx = fx / spriteR * half + half;
            const float sny = fy / spriteR * half + half;

            // Bilineal -- 4 texels vecinos
            const int sx0 = static_cast<int>(snx);
            const int sy0 = static_cast<int>(sny);
            const int sx1 = sx0 + 1;
            const int sy1 = sy0 + 1;
            if (sx0 < 0 || sy0 < 0 || sx1 >= S || sy1 >= S) continue;

            const float u = snx - static_cast<float>(sx0);
            const float v = sny - static_cast<float>(sy0);
            const float a00 = sparkSprite_[static_cast<size_t>(sy0) * S + sx0];
            const float a10 = sparkSprite_[static_cast<size_t>(sy0) * S + sx1];
            const float a01 = sparkSprite_[static_cast<size_t>(sy1) * S + sx0];
            const float a11 = sparkSprite_[static_cast<size_t>(sy1) * S + sx1];
            const float rawAlpha = (a00 * (1.0f - u) + a10 * u) * (1.0f - v)
                                 + (a01 * (1.0f - u) + a11 * u) * v;
            if (rawAlpha < 0.004f) continue;

            // Radio normalizado para saber en que capa estamos.
            const float r = std::sqrt(fx * fx + fy * fy) / spriteR;

            // Color y alpha final segun zona:
            float cr, cg, cb, alpha;
            if (r < 0.15f) {
                // Nucleo: blanco azulado brillante
                alpha = rawAlpha * coreIntensity;
                cr = 1.00f; cg = 0.96f; cb = 1.00f;
            } else if (r < 0.70f) {
                // Halo: blanco calido que se desvanece hacia naranja muy suave
                // en el borde exterior (se mezcla con el color de la llama).
                alpha = rawAlpha * haloIntensity;
                const float fade = (r - 0.15f) / 0.55f;   // [0,1] dentro del halo
                cr = 1.00f;
                cg = 1.00f - 0.08f * fade;
                cb = 0.95f - 0.15f * fade;
            } else {
                // Anillo: blanco azulado intenso con el pulso
                alpha = rawAlpha * ringIntensity;
                cr = 0.90f; cg = 0.95f; cb = 1.00f;
            }

            // Compositing aditivo: suma luz sobre el pixel existente.
            // Esto respeta el look HDR del renderer (no clampa hasta gamma).
            const size_t   offset = static_cast<size_t>(y) * pitch_ + x;
            const uint32_t back   = pixels_[offset];
            const int backR = static_cast<int>((back >> 16) & 0xFF);
            const int backG = static_cast<int>((back >>  8) & 0xFF);
            const int backB = static_cast<int>( back        & 0xFF);

            const int mr = std::min(255, backR + static_cast<int>(cr * alpha * 255.0f));
            const int mg = std::min(255, backG + static_cast<int>(cg * alpha * 255.0f));
            const int mb = std::min(255, backB + static_cast<int>(cb * alpha * 255.0f));
            pixels_[offset] = 0xFF000000u | (static_cast<uint32_t>(mr) << 16)
                                          | (static_cast<uint32_t>(mg) <<  8)
                                          |  static_cast<uint32_t>(mb);
        }
    }
}

void Renderer::drawText(int x, int y, int pixelSize, const std::string& text,
                        uint8_t r, uint8_t g, uint8_t b) {
    const uint32_t color = 0xFF000000u | (static_cast<uint32_t>(r) << 16) |
                           (static_cast<uint32_t>(g) << 8) | b;
    int cursorX = x;
    for (const char character : text) {
        const uint8_t* glyph = glyphFor(character);
        if (glyph != nullptr) {
            for (int row = 0; row < 7; ++row) {
                for (int col = 0; col < 5; ++col) {
                    if ((glyph[row] & (0x10 >> col)) == 0) continue;
                    for (int sy = 0; sy < pixelSize; ++sy) {
                        const int py = y + row * pixelSize + sy;
                        if (py < 0 || py >= height_) continue;
                        for (int sx = 0; sx < pixelSize; ++sx) {
                            const int px = cursorX + col * pixelSize + sx;
                            if (px < 0 || px >= width_) continue;
                            pixels_[static_cast<size_t>(py) * pitch_ + px] = color;
                        }
                    }
                }
            }
        }
        cursorX += 6 * pixelSize;
    }
}

void Renderer::drawHud(const FireSystem& fire, float fps) {
    const int size   = std::max(2, height_ / 260);
    const int margin = 10 * size;

    const std::string fpsLine = "FPS " + oneDecimal(fps);
    drawText(margin + size, margin + size, size * 2, fpsLine, 0, 0, 0);
    drawText(margin, margin, size * 2, fpsLine, 255, 236, 190);

    const std::string infoLine =
        "N " + std::to_string(fire.particles().size()) + "   " +
        std::to_string(width_) + "X" + std::to_string(height_) + "   TEMP " +
        oneDecimal(fire.averageTemperature());
    const int infoY = margin + 16 * size;
    drawText(margin + 1, infoY + 1, size, infoLine, 0, 0, 0);
    drawText(margin, infoY, size, infoLine, 210, 160, 110);

    drawText(margin, infoY + 9 * size, size, "ESC O Q PARA SALIR", 130, 95, 70);
}

void Renderer::drawFrame(const FireSystem& fire, float fps, int sparkIndex) {
    // La mezcla de paleta (fuego <-> arcoiris) sigue una onda triangular en
    // el tiempo: sube de 0 a 1 durante la primera mitad del ciclo, baja de
    // 1 a 0 en la segunda mitad. Esto hace que la fogata alterne entre su
    // paleta normal y un arcoiris completo de forma gradual y continua, sin
    // saltos de color de un frame a otro.
    const float cyclePos = std::fmod(fire.elapsed(), kRainbowCycleSeconds) / kRainbowCycleSeconds;
    paletteMix_ = 1.0f - std::fabs(cyclePos * 2.0f - 1.0f);

    accumulateStars(fire.elapsed());
    accumulateParticles(fire);
    buildBloomRaw();

    void* locked = nullptr;
    int   bytePitch = 0;
    if (SDL_LockTexture(texture_, nullptr, &locked, &bytePitch) == 0) {
        pixels_ = static_cast<uint32_t*>(locked);
        pitch_  = bytePitch / static_cast<int>(sizeof(uint32_t));

        composite(fire.flicker() * intensity_);
        drawGround(fire);
        drawTrees(fire);
        drawLogs(fire);
        drawStones(fire);
        drawSparkHighlight(fire, sparkIndex);
        drawHud(fire, fps);

        SDL_UnlockTexture(texture_);
        pixels_ = nullptr;
    }

    blurBloom();

    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}