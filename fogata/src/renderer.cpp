// renderer.cpp — Render por software: campo de luz flotante -> pixeles.
//
// Tuberia de un frame:
//   1. accumulateParticles() suma el brillo de cada particula al campo RGB
//   2. accumulateHearth()    suma las brasas y el halo del lecho de la fogata
//   3. toneMap()             pasa el campo a ARGB de 8 bits y, de paso, atenua
//                            el campo para que el frame siguiente arrastre
//                            estelas de humo y chispas
//   4. drawLogs()            dibuja los lenos en primer plano
//   5. drawHud()             FPS, N y resolucion sobre la imagen final
#include "renderer.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

// --- Ajustes de aspecto ---
constexpr float kTrailDecay   = 0.50f;  // cuanto sobrevive el frame anterior
constexpr float kExposure     = 0.95f;  // exposicion antes del mapeo de tonos
constexpr float kEmberBoost   = 3.20f;  // las chispas brillan mas que las llamas
constexpr float kFlameBoost   = 0.55f;
constexpr int   kReferenceN   = 4000;   // N para el que esta calibrado el brillo
constexpr float kMaxStretch   = 1.85f;  // alargamiento vertical maximo de una llama
constexpr int   kGammaLutSize = 1024;   // resolucion de la curva gamma precalculada

// Puntos de control de la paleta de cuerpo negro, de frio a incandescente.
// Cada fila es {temperatura, R, G, B} en intensidad lineal.
constexpr float kRamp[][4] = {
    {0.00f, 0.05f, 0.005f, 0.010f},  // rescoldo casi apagado
    {0.12f, 0.35f, 0.030f, 0.010f},  // rojo profundo
    {0.30f, 0.90f, 0.140f, 0.020f},  // rojo naranja
    {0.50f, 1.00f, 0.380f, 0.050f},  // naranja
    {0.70f, 1.00f, 0.640f, 0.150f},  // ambar
    {0.86f, 1.00f, 0.850f, 0.420f},  // amarillo
    {1.00f, 1.00f, 0.920f, 0.620f},  // amarillo incandescente
};
constexpr int kRampSize = static_cast<int>(sizeof(kRamp) / sizeof(kRamp[0]));

// --- Fuente de mapa de bits 5x7 para el HUD ---
// Evita depender de SDL_ttf: cada glifo son 7 bytes y cada byte una fila,
// con el bit 4 como columna izquierda.
const char kCharset[] = " .,:-/=%0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const uint8_t kGlyphs[][7] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00},  // espacio
    {0x00,0x00,0x00,0x00,0x00,0x00,0x04},  // punto
    {0x00,0x00,0x00,0x00,0x00,0x04,0x08},  // coma
    {0x00,0x00,0x04,0x00,0x04,0x00,0x00},  // dos puntos
    {0x00,0x00,0x00,0x0E,0x00,0x00,0x00},  // guion
    {0x01,0x01,0x02,0x04,0x08,0x10,0x10},  // barra
    {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00},  // igual
    {0x11,0x12,0x02,0x04,0x08,0x09,0x11},  // porcentaje
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},  // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},  // 1
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},  // 2
    {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E},  // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},  // 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},  // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},  // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},  // 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},  // 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},  // 9
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},  // A
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},  // B
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},  // C
    {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C},  // D
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},  // E
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},  // F
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},  // G
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},  // H
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},  // I
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C},  // J
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},  // K
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},  // L
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},  // M
    {0x11,0x11,0x19,0x15,0x13,0x11,0x11},  // N
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},  // O
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},  // P
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},  // Q
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},  // R
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},  // S
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},  // T
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},  // U
    {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},  // V
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11},  // W
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},  // X
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},  // Y
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},  // Z
};

// Devuelve el glifo de un caracter, o nullptr si no esta en el juego soportado.
const uint8_t* glyphFor(char c) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    if (c == '\0') return nullptr;
    const char* found = std::strchr(kCharset, c);
    if (found == nullptr) return nullptr;
    return kGlyphs[found - kCharset];
}

// Convierte un flotante a texto con un decimal, sin depender de <sstream>.
std::string oneDecimal(float value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.1f", static_cast<double>(value));
    return buffer;
}

}  // namespace

Renderer::~Renderer() {
    // Destruccion en orden inverso a la creacion; SDL_Quit lo llama main().
    if (texture_  != nullptr) SDL_DestroyTexture(texture_);
    if (renderer_ != nullptr) SDL_DestroyRenderer(renderer_);
    if (window_   != nullptr) SDL_DestroyWindow(window_);
    texture_  = nullptr;
    renderer_ = nullptr;
    window_   = nullptr;
}

bool Renderer::init(const Config& cfg, std::string& error) {
    width_     = cfg.width;
    height_    = cfg.height;
    intensity_ = cfg.intensity;

    // Con muchas particulas cada una debe aportar menos luz, o la pantalla se
    // satura a blanco. Normalizamos el brillo contra un N de referencia.
    densidad_ = std::min(3.0f, std::max(0.10f,
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

    field_.assign(static_cast<size_t>(width_) * height_ * 3, 0.0f);
    pixels_.assign(static_cast<size_t>(width_) * height_, 0u);

    // Precalculo de la paleta: interpolacion lineal entre los puntos de control.
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

    // Precalculo de la curva gamma. El mapeo de tonos necesitaria un pow() por
    // canal y por pixel: a 1280x720 son casi 3 millones de llamadas por frame,
    // medidas como el mayor coste fijo del render. Con la tabla queda en una
    // division y una lectura, y el coste por frame baja a menos de la mitad.
    for (int i = 0; i < kGammaLutSize; ++i) {
        const float mapped = static_cast<float>(i) / (kGammaLutSize - 1);
        const float gamma  = std::pow(mapped, 1.0f / 2.2f);
        gammaLut_[i] = static_cast<uint8_t>(gamma * 255.0f + 0.5f);
    }
    return true;
}

void Renderer::accumulateParticles(const FireSystem& fire) {
    const float exposure = intensity_ * densidad_;

    for (const Particle& p : fire.particles()) {
        // El brillo emitido crece mas rapido que la temperatura (ley de
        // radiacion): una particula tibia casi no aporta luz.
        const float emission = p.temp * p.temp *
                               (p.isEmber ? kEmberBoost : kFlameBoost) * exposure;
        if (emission <= 0.008f) continue;  // descarta particulas ya apagadas

        const int   index  = std::min(255, static_cast<int>(p.temp * 255.0f));
        const float colorR = blackbody_[index][0] * p.tintR * emission;
        const float colorG = blackbody_[index][1] * p.tintG * emission;
        const float colorB = blackbody_[index][2] * p.tintB * emission;

        // El brillo se estira verticalmente en funcion de la velocidad de
        // ascenso: es lo que convierte manchas redondas en lenguas de fuego.
        const float stretch = std::min(kMaxStretch, 1.0f + std::fabs(p.vy) * 0.006f);
        const float radiusX = p.radius;
        const float radiusY = p.radius * stretch;
        const float invRadiusXSq = 1.0f / (radiusX * radiusX);
        const float invRadiusYSq = 1.0f / (radiusY * radiusY);

        // Recorte de la elipse de brillo contra los bordes de la ventana.
        const int x0 = std::max(0,           static_cast<int>(p.x - radiusX));
        const int x1 = std::min(width_  - 1, static_cast<int>(p.x + radiusX));
        const int y0 = std::max(0,           static_cast<int>(p.y - radiusY));
        const int y1 = std::min(height_ - 1, static_cast<int>(p.y + radiusY));

        for (int y = y0; y <= y1; ++y) {
            const float dy   = static_cast<float>(y) - p.y;
            const float dySq = dy * dy * invRadiusYSq;
            if (dySq >= 1.0f) continue;
            float* row = &field_[(static_cast<size_t>(y) * width_ + x0) * 3];
            for (int x = x0; x <= x1; ++x, row += 3) {
                const float dx = static_cast<float>(x) - p.x;
                const float d2 = dx * dx * invRadiusXSq + dySq;
                if (d2 >= 1.0f) continue;
                // Caida suave (1 - d^2)^2: nucleo brillante, borde difuso.
                const float falloff = 1.0f - d2;
                const float weight  = falloff * falloff;
                row[0] += colorR * weight;
                row[1] += colorG * weight;
                row[2] += colorB * weight;
            }
        }
    }
}

void Renderer::accumulateHearth(const FireSystem& fire) {
    // Halo ancho y calido alrededor del lecho de brasas: da la sensacion de
    // que la fogata ilumina el entorno en lugar de flotar en el vacio.
    const float glow     = fire.emberGlow() * intensity_;
    const float centerX  = fire.hearthX();
    const float centerY  = fire.hearthY();
    const float radius   = fire.hearthHalfWidth() * 3.2f;
    const float radiusSq = radius * radius;

    const int x0 = std::max(0,           static_cast<int>(centerX - radius));
    const int x1 = std::min(width_  - 1, static_cast<int>(centerX + radius));
    const int y0 = std::max(0,           static_cast<int>(centerY - radius));
    const int y1 = std::min(height_ - 1, static_cast<int>(centerY + radius));

    for (int y = y0; y <= y1; ++y) {
        const float dy = static_cast<float>(y) - centerY;
        // El halo se aplasta verticalmente: la luz se derrama sobre el suelo.
        const float dySq = (dy * 1.6f) * (dy * 1.6f);
        float* row = &field_[(static_cast<size_t>(y) * width_ + x0) * 3];
        for (int x = x0; x <= x1; ++x, row += 3) {
            const float dx = static_cast<float>(x) - centerX;
            const float d2 = dx * dx + dySq;
            if (d2 >= radiusSq) continue;
            const float falloff = 1.0f - d2 / radiusSq;
            const float weight  = falloff * falloff * falloff * glow;
            row[0] += 0.42f * weight;
            row[1] += 0.14f * weight;
            row[2] += 0.03f * weight;
        }
    }
}

void Renderer::toneMap() {
    // Mapeo de tonos de Reinhard, c/(1+c), seguido de correccion gamma: el
    // primero comprime el rango dinamico sin recortar (por eso el nucleo se ve
    // claro y el halo naranja en lugar de un borron blanco) y la segunda pasa
    // de intensidad lineal a la respuesta de un monitor.
    //
    // La curva gamma sale de la tabla precalculada en init(): sustituir el
    // pow() por canal por una lectura de memoria fue la optimizacion mas
    // rentable de todo el render secuencial.
    for (int y = 0; y < height_; ++y) {
        // Fondo nocturno: azul apagado arriba, un poco mas calido abajo.
        const float skyT = static_cast<float>(y) / static_cast<float>(height_);
        const float ambient[3] = {
            0.006f + 0.010f * skyT,
            0.006f + 0.006f * skyT,
            0.026f - 0.012f * skyT,
        };

        float*    src = &field_[static_cast<size_t>(y) * width_ * 3];
        uint32_t* dst = &pixels_[static_cast<size_t>(y) * width_];

        for (int x = 0; x < width_; ++x, src += 3) {
            uint32_t argb = 0xFF000000u;
            for (int c = 0; c < 3; ++c) {
                const float value  = src[c] * kExposure + ambient[c];
                const float mapped = value / (1.0f + value);
                const int   index  = static_cast<int>(mapped * (kGammaLutSize - 1));
                argb |= static_cast<uint32_t>(gammaLut_[index]) << (16 - 8 * c);
                // Atenuacion de la estela hecha en el mismo recorrido: evita
                // una segunda pasada completa sobre el campo cada frame.
                src[c] *= kTrailDecay;
            }
            dst[x] = argb;
        }
    }
}

void Renderer::drawLogs(const FireSystem& fire) {
    // Tres lenos cruzados en la base, dibujados como segmentos gruesos ya
    // sobre la imagen final para que tapen la llama y aporten profundidad.
    const float cx    = fire.hearthX();
    const float cy    = fire.hearthY() + fire.hearthHalfWidth() * 0.45f;
    const float span  = fire.hearthHalfWidth() * 2.1f;
    const float thick = fire.hearthHalfWidth() * 0.30f;
    const float glow  = fire.emberGlow();

    struct Segment { float x0, y0, x1, y1; };
    const Segment logs[3] = {
        {cx - span,         cy + thick * 0.6f, cx + span * 0.85f, cy - thick * 0.5f},
        {cx - span * 0.80f, cy - thick * 0.7f, cx + span,         cy + thick * 0.5f},
        {cx - span * 0.35f, cy + thick * 1.1f, cx + span * 0.40f, cy + thick * 1.3f},
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
                // Distancia del pixel al segmento (proyeccion acotada a [0,1]).
                const float px = static_cast<float>(x) - seg.x0;
                const float py = static_cast<float>(y) - seg.y0;
                float t = (px * dx + py * dy) / lenSq;
                t = std::min(1.0f, std::max(0.0f, t));
                const float ox   = px - dx * t;
                const float oy   = py - dy * t;
                const float dist = std::sqrt(ox * ox + oy * oy);
                if (dist > thick) continue;

                // Las brasas viven en el centro del leno: cuanto mas cerca del
                // eje de la fogata, mas incandescente se ve la madera.
                const float toCenter = std::fabs(static_cast<float>(x) - cx) / span;
                const float heat = glow * std::max(0.0f, 1.0f - toCenter) *
                                   (1.0f - dist / thick) * 0.9f;
                const float shade = 0.35f + 0.65f * (1.0f - dist / thick);

                const int r = std::min(255, static_cast<int>(26.0f * shade + heat * 235.0f));
                const int g = std::min(255, static_cast<int>(16.0f * shade + heat * 105.0f));
                const int b = std::min(255, static_cast<int>(12.0f * shade + heat *  30.0f));

                // Borde suavizado: el ultimo 20% del grosor se mezcla con el fondo.
                const float alpha = std::min(1.0f, (thick - dist) / (thick * 0.2f + 1e-3f));
                const size_t offset = static_cast<size_t>(y) * width_ + x;
                const uint32_t dst = pixels_[offset];
                const int dr = static_cast<int>((dst >> 16) & 0xFF);
                const int dg = static_cast<int>((dst >>  8) & 0xFF);
                const int db = static_cast<int>( dst        & 0xFF);
                const int mr = static_cast<int>(dr + (r - dr) * alpha);
                const int mg = static_cast<int>(dg + (g - dg) * alpha);
                const int mb = static_cast<int>(db + (b - db) * alpha);
                pixels_[offset] = 0xFF000000u |
                                  (static_cast<uint32_t>(mr) << 16) |
                                  (static_cast<uint32_t>(mg) <<  8) |
                                   static_cast<uint32_t>(mb);
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
                    // Cada punto del glifo se expande a un bloque solido.
                    for (int sy = 0; sy < pixelSize; ++sy) {
                        const int py = y + row * pixelSize + sy;
                        if (py < 0 || py >= height_) continue;
                        for (int sx = 0; sx < pixelSize; ++sx) {
                            const int px = cursorX + col * pixelSize + sx;
                            if (px < 0 || px >= width_) continue;
                            pixels_[static_cast<size_t>(py) * width_ + px] = color;
                        }
                    }
                }
            }
        }
        cursorX += 6 * pixelSize;  // 5 columnas de glifo + 1 de separacion
    }
}

void Renderer::drawHud(const FireSystem& fire, float fps) {
    const int size   = std::max(2, height_ / 260);
    const int margin = 10 * size;

    // Sombra negra desplazada para que el texto se lea sobre la llama.
    const std::string fpsLine = "FPS " + oneDecimal(fps);
    drawText(margin + size, margin + size, size * 2, fpsLine, 0, 0, 0);
    drawText(margin, margin, size * 2, fpsLine, 255, 236, 190);

    const std::string infoLine =
        "N " + std::to_string(fire.particles().size()) + "   " +
        std::to_string(width_) + "X" + std::to_string(height_) + "   SECUENCIAL";
    const int infoY = margin + 16 * size;
    drawText(margin + 1, infoY + 1, size, infoLine, 0, 0, 0);
    drawText(margin, infoY, size, infoLine, 210, 160, 110);

    drawText(margin, infoY + 9 * size, size, "ESC O Q PARA SALIR", 130, 95, 70);
}

void Renderer::drawFrame(const FireSystem& fire, float fps) {
    accumulateParticles(fire);
    accumulateHearth(fire);
    toneMap();
    drawLogs(fire);
    drawHud(fire, fps);

    SDL_UpdateTexture(texture_, nullptr, pixels_.data(),
                      width_ * static_cast<int>(sizeof(uint32_t)));
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}
