#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include "body.h"

// Wrapper simple sobre SDL2 para inicializar la ventana y dibujar los cuerpos.
//
// Entregable Semana 1: setup de SDL + ventana basica + loop de render vacio
// (clear -> present -> handleEvents, sin cuerpos todavia). drawBody() ya
// esta implementada pero se conecta al loop principal hasta que el modulo
// de fisica (Persona A) y el de inicializacion de galaxia (Persona C) esten
// integrados en main.cpp (semana 2).
class Renderer {
public:
    // Inicializa SDL_Init, crea la ventana y el SDL_Renderer asociado.
    // Retorna false si SDL o la creacion de la ventana fallan (programacion
    // defensiva: nunca asumimos que SDL_Init siempre funciona).
    bool init(int width, int height, const char* title);

    // Limpia el canvas con un color de fondo tipo "espacio" (casi negro).
    void clear();

    // Dibuja un cuerpo como un circulo relleno en su posicion actual,
    // usando su color y radio. Usa el algoritmo de punto medio para
    // trazar el circulo y lo rellena con lineas horizontales.
    void drawBody(const Body& body);

    // Presenta en pantalla todo lo dibujado desde el ultimo clear().
    void present();

    // Procesa la cola de eventos de SDL (cerrar ventana, tecla ESC).
    // Retorna false si el programa debe terminar.
    bool handleEvents();

    // Libera los recursos de SDL (renderer, ventana, SDL_Quit).
    void shutdown();

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* sdlRenderer = nullptr;
};

#endif // RENDERER_H
