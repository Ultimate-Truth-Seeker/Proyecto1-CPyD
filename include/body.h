#ifndef BODY_H
#define BODY_H

// Representa un cuerpo (particula) en la simulacion de N-cuerpos.
// Cada cuerpo tiene posicion, velocidad, masa y un color RGB para el render.
struct Body {
    double x, y;             // Posicion (unidades arbitrarias de simulacion)
    double vx, vy;            // Velocidad
    double mass;               // Masa (usada en el calculo de fuerza gravitacional)
    unsigned char r, g, b;      // Color RGB para dibujar el cuerpo
    double radius;               // Radio visual (solo para el render, no afecta la fisica)
};

// Asigna un color RGB pseudoaleatorio a un cuerpo.
// Se usa al inicializar la galaxia para que cada cuerpo se distinga visualmente.
void assignRandomColor(Body& b);

#endif // BODY_H
