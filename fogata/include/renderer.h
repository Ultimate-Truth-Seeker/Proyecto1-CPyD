// renderer.h — Render por software del fuego sobre una textura de SDL2.
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

// Dibuja la fogata acumulando luz en un campo RGB de coma flotante y luego
// mapeandolo a colores de 8 bits. Trabajar en flotante permite sumar el brillo
// de miles de particulas superpuestas sin saturar el color hasta el final,
// que es lo que produce el nucleo blanco incandescente y el halo naranja.
//
// Es duena de todos los recursos de SDL que crea y los libera en el destructor.
class Renderer {
 public:
    Renderer() = default;
    ~Renderer();

    // No copiable: posee punteros de SDL.
    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Crea ventana, renderer y textura de streaming.
    // Retorna false y llena 'error' si SDL no pudo inicializar algun recurso.
    bool init(const Config& cfg, std::string& error);

    // Dibuja un frame completo y lo presenta en pantalla.
    // Entradas: 'fire' estado fisico actual, 'fps' promedio a mostrar en el HUD.
    void drawFrame(const FireSystem& fire, float fps);

 private:
    void accumulateParticles(const FireSystem& fire);  // suma el brillo de cada particula
    void accumulateHearth(const FireSystem& fire);     // brasas y halo del hogar
    void toneMap();                           // campo flotante -> ARGB + estelas
    void drawLogs(const FireSystem& fire);    // lenos en primer plano
    void drawHud(const FireSystem& fire, float fps);
    // Escribe texto con la fuente de mapa de bits de 5x7 incluida en el .cpp.
    void drawText(int x, int y, int pixelSize, const std::string& text,
                  uint8_t r, uint8_t g, uint8_t b);

    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture*  texture_  = nullptr;

    int   width_      = 0;
    int   height_     = 0;
    float intensity_  = 1.0f;
    float densidad_   = 1.0f;  // normaliza el brillo frente al valor de N

    std::vector<float>    field_;   // acumulador lineal RGB, 3 flotantes por pixel
    std::vector<uint32_t> pixels_;  // resultado ARGB8888 que se sube a la textura
    float   blackbody_[256][3];     // paleta de cuerpo negro indexada por temperatura
    uint8_t gammaLut_[1024];        // curva gamma precalculada para el mapeo de tonos
};

#endif  // FOGATA_RENDERER_H
