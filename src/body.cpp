#include "body.h"
#include <cstdlib>

void assignRandomColor(Body& b) {
    // Colores pseudoaleatorios en rango completo [0,255] por canal.
    // Se evitan colores demasiado oscuros (cerca de negro) para que
    // los cuerpos se vean bien contra el fondo del canvas.
    b.r = static_cast<unsigned char>(80 + (rand() % 176));
    b.g = static_cast<unsigned char>(80 + (rand() % 176));
    b.b = static_cast<unsigned char>(80 + (rand() % 176));
}
