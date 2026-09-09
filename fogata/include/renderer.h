#ifndef FOGATA_RENDERER_H
#define FOGATA_RENDERER_H

#include <cstdint>
#include <string>
#include <vector>

#include "config.h"
#include "particle.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

struct Star {
    int   x;
    int   y;
    float brightness;
    float phase;
    float speed;   // fraccion de velocidad de paralaje relativa al fondo
};

struct Tree {
    float x;       // posicion X base del tronco (fraccion de width)
    float scale;   // escala relativa (simula profundidad)
    int   layers;  // numero de capas del pino (3-5)
    float lean;    // inclinacion lateral suave
};

class Renderer {
 public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool init(const Config& cfg, const FireSystem& fire, std::string& error);

    // sparkIndex es el indice devuelto por FireSystem::findCriticalSpark(),
    // o -1 si ninguna particula supero el umbral en este frame. Cuando es
    // >= 0, esa particula especifica se dibuja con un color/destello
    // distintivo (ver drawSparkHighlight()) ademas de su color normal.
    void drawFrame(const FireSystem& fire, float fps, int sparkIndex = -1);

 private:
    void buildPalette();
    void buildNightSky(Rng& rng);
    void buildFirelight(const FireSystem& fire);
    void buildSparkSprite();
    void accumulateStars(float time);
    void accumulateParticles(const FireSystem& fire);
    void buildBloomRaw();
    void composite(float glow);
    void blurBloom();
    void drawStones(const FireSystem& fire);
    void drawLogs(const FireSystem& fire);
    void drawGround(const FireSystem& fire);
    void drawTrees(const FireSystem& fire);
    void drawSparkHighlight(const FireSystem& fire, int sparkIndex);
    void drawHud(const FireSystem& fire, float fps);
    void drawText(int x, int y, int pixelSize, const std::string& text,
                  uint8_t r, uint8_t g, uint8_t b);

    // Devuelve el color RGB (en [0,1]) para una temperatura dada, mezclando
    // la paleta de cuerpo negro fija (blackbody_) con una paleta arcoiris
    // que rota su matiz (hue) con el tiempo, segun paletteMix_. Con
    // paletteMix_ = 0 el resultado es 100% cuerpo negro (el look original);
    // con paletteMix_ = 1 es 100% arcoiris.
    void colorForTemperature(float temp, float time, float* outR, float* outG, float* outB) const;

    // Sprite radial precalculado para el highlight de la chispa critica.
    // Contiene alpha en [0,1] para un radio normalizado; se estampa cada
    // frame escalando al radio real de la particula + pulso de tiempo.
    // Dimension fija; se recalcula en buildSparkSprite().
    static constexpr int kSparkSpriteSize = 64;
    float sparkSprite_[kSparkSpriteSize * kSparkSpriteSize] = {};

    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture*  texture_  = nullptr;

    int   width_     = 0;
    int   height_    = 0;
    float intensity_ = 1.0f;
    float density_   = 1.0f;

    std::vector<float>    field_;
    std::vector<float>    background_;
    std::vector<float>    firelight_;
    std::vector<float>    bloom_;
    std::vector<float>    bloomRaw_;
    std::vector<float>    bloomScratch_;
    std::vector<Star>     stars_;
    std::vector<Tree>     trees_;
    std::vector<std::vector<int>> particleTiles_;

    uint32_t* pixels_ = nullptr;
    int pitch_ = 0;

    int bloomWidth_  = 0;
    int bloomHeight_ = 0;
    int particleTileWidth_ = 0;
    int particleTileHeight_ = 0;

    float   blackbody_[256][3];
    uint8_t gammaLut_[1024];

    // Cuanto de la paleta arcoiris se mezcla sobre la de cuerpo negro,
    // en [0,1]. Se recalcula cada frame en drawFrame() como una onda
    // triangular lenta, para que la fogata alterne entre su paleta de
    // fuego normal y un arcoiris completo, en vez de saltar de golpe.
    float paletteMix_ = 0.0f;
};

#endif  // FOGATA_RENDERER_H
