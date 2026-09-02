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
    int   pixel;
    float brightness;
    float phase;
};

class Renderer {
 public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool init(const Config& cfg, const FireSystem& fire, std::string& error);
    void drawFrame(const FireSystem& fire, float fps);

 private:
    void buildPalette();
    void buildNightSky(Rng& rng);
    void buildFirelight(const FireSystem& fire);
    void accumulateStars(float time);
    void accumulateParticles(const FireSystem& fire);
    void composite(float glow);
    void blurBloom();
    void drawStones(const FireSystem& fire);
    void drawLogs(const FireSystem& fire);
    void drawHud(const FireSystem& fire, float fps);
    void drawText(int x, int y, int pixelSize, const std::string& text,
                  uint8_t r, uint8_t g, uint8_t b);

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

    uint32_t* pixels_ = nullptr;
    int pitch_ = 0;

    int bloomWidth_  = 0;
    int bloomHeight_ = 0;

    float   blackbody_[256][3];
    uint8_t gammaLut_[1024];
};

#endif  // FOGATA_RENDERER_H
