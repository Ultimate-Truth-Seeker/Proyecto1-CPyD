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

    Rng rng(cfg.seed ^ 0xA5A5A5A5u);
    buildPalette();
    buildNightSky(rng);
    buildFirelight(fire);
    return true;
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
        const float fade = 1.0f - 0.45f * depth;

        for (int x = 0; x < width_; ++x) {
            const float dim   = vignetteAt(x, y, width_, height_);
            const float grain = 0.75f + 0.50f * rng.nextFloat();
            const float soil[3] = {
                0.0062f * grain * fade,
                0.0044f * grain * fade,
                0.0034f * grain * fade,
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
        stars_.push_back({y * width_ + x,
                          rng.range(0.10f, 0.85f) * (0.45f + 0.55f * far) *
                              vignetteAt(x, y, width_, height_),
                          rng.range(0.0f, 6.2831853f)});
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
    for (const Star& star : stars_) {
        const float twinkle = star.brightness *
                              (0.55f + 0.45f * std::sin(time * 1.7f + star.phase));
        float* out = &field_[static_cast<size_t>(star.pixel) * 3];
        out[0] += kStarColor[0] * twinkle;
        out[1] += kStarColor[1] * twinkle;
        out[2] += kStarColor[2] * twinkle;
    }
}

void Renderer::accumulateParticles(const FireSystem& fire) {
    const float exposure  = intensity_ * density_;
    const float hearthY   = fire.hearthY();
    const float blueRange = 70.0f * fire.scale();

    #pragma omp parallel for schedule(dynamic, 64)
    for (int pi = 0; pi < static_cast<int>(fire.particles().size()); ++pi) {
        const Particle& p = fire.particles()[pi];
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

            const int index = std::min(255, static_cast<int>(p.temp * 255.0f));
            colorR = blackbody_[index][0] * p.tintR * emission;
            colorG = blackbody_[index][1] * p.tintG * emission;
            colorB = blackbody_[index][2] * p.tintB * emission;

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

        const int x0 = std::max(0,           static_cast<int>(p.x - radiusX));
        const int x1 = std::min(width_  - 1, static_cast<int>(p.x + radiusX));
        const int y0 = std::max(0,           static_cast<int>(p.y - radiusY));
        const int y1 = std::min(height_ - 1, static_cast<int>(p.y + radiusY));

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

void Renderer::composite(float glow) {
    #pragma omp parallel for
    for (int y = 0; y < height_; ++y) {
        const int blockY = y / kBloomScale;
        float* raw = &bloomRaw_[static_cast<size_t>(blockY) * bloomWidth_ * 3];
        if (y % kBloomScale == 0) {
            std::fill(raw, raw + static_cast<size_t>(bloomWidth_) * 3, 0.0f);
        }

        const float* halo = &bloom_[static_cast<size_t>(blockY) * bloomWidth_ * 3];
        const float* bg   = &background_[static_cast<size_t>(y) * width_ * 3];
        const float* lit  = &firelight_[static_cast<size_t>(y) * width_];
        float*       src  = &field_[static_cast<size_t>(y) * width_ * 3];
        uint32_t*    dst  = &pixels_[static_cast<size_t>(y) * pitch_];

        int blockX = 0;
        int blockStep = 0;

        for (int x = 0; x < width_; ++x, src += 3, bg += 3) {
            const float* glowRow = halo + blockX * 3;
            float*       acc     = raw + blockX * 3;
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
                acc[c] += light;
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

void Renderer::drawFrame(const FireSystem& fire, float fps) {
    accumulateStars(fire.elapsed());
    accumulateParticles(fire);

    void* locked = nullptr;
    int   bytePitch = 0;
    if (SDL_LockTexture(texture_, nullptr, &locked, &bytePitch) == 0) {
        pixels_ = static_cast<uint32_t*>(locked);
        pitch_  = bytePitch / static_cast<int>(sizeof(uint32_t));

        composite(fire.flicker() * intensity_);
        drawLogs(fire);
        drawStones(fire);
        drawHud(fire, fps);

        SDL_UnlockTexture(texture_);
        pixels_ = nullptr;
    }

    blurBloom();

    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}