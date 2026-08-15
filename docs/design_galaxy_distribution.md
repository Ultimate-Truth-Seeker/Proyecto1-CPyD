# Investigación: distribución inicial de una galaxia (Persona C — inicio Semana 1)

## Objetivo
Definir cómo se generan las posiciones, velocidades y masas iniciales de los
N cuerpos para que la simulación se vea y se comporte como una galaxia con
un núcleo central, en vez de un conjunto de puntos aleatorios sin estructura.

## Modelo elegido: disco con núcleo central masivo

- **Cuerpo 0 (núcleo):** un solo cuerpo con masa mucho mayor que el resto
  (ej. 5,000–10,000x la masa promedio de un cuerpo normal), ubicado en el
  centro del canvas, con velocidad inicial cero. Actúa como el "agujero
  negro central" alrededor del cual orbitan los demás cuerpos.

- **Cuerpos 1..N-1 (estrellas del disco):** distribuidos en un disco
  alrededor del núcleo:
  - Radio `r` elegido pseudoaleatoriamente en un rango `[r_min, r_max]`
    (ej. entre el 5% y el 45% del lado más corto del canvas), para dejar un
    hueco vacío cerca del núcleo y evitar que los cuerpos se amontonen ahí.
  - Ángulo `theta` elegido pseudoaleatoriamente en `[0, 2π)`.
  - Posición: `x = centro_x + r*cos(theta)`, `y = centro_y + r*sin(theta)`.

## Velocidad orbital (la parte de física/trigonometría)

Para que las estrellas *orbiten* el núcleo en vez de caer directo hacia él o
salir disparadas, se les da una velocidad **tangencial** (perpendicular al
vector radio) cuya magnitud aproxima una órbita circular estable:

```
v_orbital = sqrt(G * M_nucleo / r)
```

Esto viene de igualar la fuerza gravitacional con la fuerza centrípeta
necesaria para mantener una órbita circular: `G*M*m/r² = m*v²/r`.

El vector de velocidad tangencial (perpendicular al radio, en sentido
antihorario) se obtiene rotando 90° el vector unitario radial:

```
vx = -v_orbital * sin(theta)
vy =  v_orbital * cos(theta)
```

Con esto, cada estrella entra a la simulación ya "orbitando" en vez de
tener que estabilizarse desde velocidad cero — es lo que le da a la
simulación su apariencia de galaxia espiral/disco en vez de una explosión
de partículas cayendo hacia el centro.

## Masas y colores

- Las estrellas del disco reciben una masa pequeña y aleatoria en un rango
  angosto (ej. 0.5–2.0 en las unidades de la simulación) para que ninguna
  perturbe demasiado a las demás — solo el núcleo domina la dinámica.
- El color de cada estrella se asigna pseudoaleatoriamente
  (`assignRandomColor`, en `body.cpp`) para cumplir el requisito de colores
  variados del enunciado.

## Referencias consultadas
- Aarseth, S. J. — *Gravitational N-Body Simulations* (métodos de softening
  y aproximación de órbitas circulares para condiciones iniciales).
- Documentación de la técnica clásica de "Plummer sphere" para inicializar
  sistemas N-body — se consideró pero se descartó por ser más compleja de
  lo que este proyecto necesita; el modelo de disco simple cubre los
  requisitos de la guía sin sobre-complicar el código.
