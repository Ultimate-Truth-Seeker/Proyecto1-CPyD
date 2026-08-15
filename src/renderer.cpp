#include "renderer.h"
#include <cstdio>

bool Renderer::init(int width, int height, const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "Error al inicializar SDL: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(title,
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width, height, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::fprintf(stderr, "Error al crear la ventana: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    sdlRenderer = SDL_CreateRenderer(window, -1,
                                      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (sdlRenderer == nullptr) {
        std::fprintf(stderr, "Error al crear el renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    return true;
}

void Renderer::clear() {
    // Fondo tipo "espacio": casi negro con un ligero tinte azulado
    SDL_SetRenderDrawColor(sdlRenderer, 5, 5, 15, 255);
    SDL_RenderClear(sdlRenderer);
}

void Renderer::drawBody(const Body& body) {
    SDL_SetRenderDrawColor(sdlRenderer, body.r, body.g, body.b, 255);

    // Algoritmo de punto medio para circulo, rellenando cada fila con una
    // linea horizontal (mas eficiente que dibujar pixel por pixel).
    int cx = static_cast<int>(body.x);
    int cy = static_cast<int>(body.y);
    int radius = static_cast<int>(body.radius);

    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        SDL_RenderDrawLine(sdlRenderer, cx - x, cy + y, cx + x, cy + y);
        SDL_RenderDrawLine(sdlRenderer, cx - x, cy - y, cx + x, cy - y);
        SDL_RenderDrawLine(sdlRenderer, cx - y, cy + x, cx + y, cy + x);
        SDL_RenderDrawLine(sdlRenderer, cx - y, cy - x, cx + y, cy - x);

        y += 1;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void Renderer::present() {
    SDL_RenderPresent(sdlRenderer);
}

bool Renderer::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
            return false;
        }
    }
    return true;
}

void Renderer::shutdown() {
    if (sdlRenderer != nullptr) {
        SDL_DestroyRenderer(sdlRenderer);
        sdlRenderer = nullptr;
    }
    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}
