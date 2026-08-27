# Arquitectura — Screensaver de fogata (version secuencial)

Documento de diseño de la primera entrega del Proyecto #1 de Computación
Paralela y Distribuida: el screensaver de una fogata, implementado en C++17 con
SDL2, **totalmente secuencial**. La versión paralela con OpenMP se construye
después sobre esta misma base.

El código vive en la carpeta `fogata/` del repositorio, separado del scaffolding
previo del simulador de galaxia, que queda intacto.

---

## 1. Qué pide el enunciado y dónde se resuelve

| Requisito del enunciado | Dónde se cumple |
|---|---|
| Recibir al menos un parámetro `N` | `-n` en [config.cpp](fogata/src/config.cpp) |
| Varios colores pseudoaleatorios | tintes por partícula en `FireSystem::respawn`, PRNG `Rng` |
| Canvas mínimo de 640x480 | validado en `limits::kMinWidth/kMinHeight` |
| Movimiento y estética | sistema de partículas + campo de luz acumulado |
| Física o trigonometría | flotabilidad, arrastre, confinamiento y turbulencia por suma de senos |
| Mostrar FPS | HUD en pantalla y reporte por consola cada segundo |
| Programación defensiva | validación completa de argumentos antes de tocar SDL |
| Evitar variables hard-coded | todo lo ajustable vive en `struct Config` |

---

## 2. Estructura de archivos

```
Proyecto1-CPyD/
├── Arquitectura.md            <- este documento
└── fogata/
    ├── Makefile               compilación, detección de SDL2, flags de optimización
    ├── README.md              dependencias, parámetros, mediciones de FPS
    ├── docs/
    │   └── captura.png        captura del screensaver en ejecución
    ├── include/
    │   ├── config.h           struct Config + contrato del parseo de argumentos
    │   ├── particle.h         struct Particle, clase Rng, clase FireSystem
    │   └── renderer.h         clase Renderer
    └── src/
        ├── config.cpp         parseo y validación de la línea de comandos
        ├── particle.cpp       modelo físico del fuego
        ├── renderer.cpp       render por software y HUD
        └── main.cpp           bucle principal y medición de FPS
```

Son **cuatro unidades de traducción** y tres cabeceras. Es deliberado: cada
archivo corresponde a una responsabilidad clara y no hay capas intermedias,
fábricas ni interfaces abstractas que no aporten nada a un programa de este
tamaño.

---

## 3. Decisiones de arquitectura

### 3.1 Tres módulos con una dependencia en una sola dirección

```
config  ──▶  particle  ──▶  renderer
   └──────────────┴──────────────┴──▶  main
```

- **`config`** no depende de nada. Es una estructura de datos y su parser.
- **`particle`** depende sólo de `config`. **No conoce SDL**: es física pura
  sobre un arreglo de partículas. Se podría probar desde consola sin abrir
  ninguna ventana.
- **`renderer`** depende de `config` y de `particle`, y es el único que incluye
  `<SDL2/SDL.h>` junto con `main`.
- **`main`** los orquesta.

La razón de fondo es la segunda entrega: cuando se paralelice con OpenMP, la
frontera entre "estado físico" y "cómo se dibuja" es exactamente la frontera
entre los dos bucles que hay que repartir entre hilos. Si la física supiera de
texturas, esa separación no existiría.

### 3.2 Número de partículas fijo, sin asignaciones durante el bucle

`FireSystem` reserva el `std::vector<Particle>` una sola vez en el constructor y
**nunca crece ni encoge**. Cuando una partícula se enfría por debajo del umbral,
no se destruye: se recicla en el lecho de brasas (`respawn`).

Consecuencias:

- Cero `new`/`delete` por frame, y por lo tanto ningún pico de latencia por
  asignación de memoria en medio de la animación.
- El recorrido del arreglo es lineal y contiguo, lo que aprovecha la caché.
- De cara a OpenMP: el arreglo tiene tamaño conocido y estable, así que el
  bucle de física es un `for` de índice entero, el caso más simple de repartir.

### 3.3 La temperatura hace de variable de vida

No hay campo `vida` ni `edad`. La temperatura normalizada `temp` cumple tres
funciones a la vez: controla la fuerza de flotabilidad, elige el color en la
paleta de cuerpo negro y determina cuándo la partícula muere. Un solo escalar
en lugar de tres, y el resultado es físicamente coherente: lo que se apaga es
lo que se ha enfriado.

### 3.4 Modelo físico

Todas las fuerzas se expresan en píxeles/segundo² y se escalan por
`scale_ = height / 720`, de modo que la fogata se ve igual a cualquier
resolución. Por partícula:

```
a_y = -B · temp^1.5          flotabilidad (el aire caliente sube; la fuerza
                             crece más rápido que la temperatura)
    + T_y(x, y, t)           turbulencia vertical
    - D · v_y                arrastre viscoso

a_x = T_x(x, y, t)           turbulencia horizontal
    + W · gust(t)            viento con ráfagas
    + C · (x_hogar - x)·(1-temp)   confinamiento
    - D · v_x                arrastre viscoso
```

Tres decisiones concretas del modelo:

1. **El campo de turbulencia es una suma de senos y cosenos evaluada en la
   posición de la partícula**, no ruido independiente por partícula. Al ser un
   campo continuo, partículas vecinas reciben empujes parecidos y se forman
   remolinos y lenguas de fuego reconocibles. Con ruido independiente el
   resultado es una nube de puntos temblando, no fuego.

2. **El confinamiento es proporcional a `(1 - temp)`**, no a `temp`. Las
   partículas frías —que son las que ya subieron— son arrastradas hacia el eje
   de la fogata, así que la llama nace ancha en la base y se cierra en punta.
   Con el signo contrario se obtiene una columna invertida que no parece fuego;
   fue uno de los ajustes que hubo que corregir tras la primera prueba visual.

3. **Integración de Euler semi-implícita** (primero velocidad, luego posición).
   Es estable para este rango de fuerzas, cuesta O(N) y no requiere guardar
   estado del paso anterior. Un integrador de mayor orden no aportaría nada
   perceptible en una animación.

Las ráfagas de viento son tres senos de períodos inconmensurables, de modo que
el patrón nunca se repite de forma evidente.

### 3.5 Render: campo de luz en coma flotante, no dibujo directo

Esta es la decisión que define el aspecto del programa. En lugar de pintar cada
partícula directamente sobre el framebuffer de 8 bits:

1. Cada partícula **suma** su brillo (color de cuerpo negro × tinte × emisión) a
   un campo RGB de `float`, con caída suave `(1 - d²)²` sobre una elipse.
2. Sólo al final, el campo completo pasa por un mapeo de tonos.

Sumar en coma flotante y comprimir el rango una única vez es lo que produce el
núcleo blanco incandescente rodeado de naranja: donde se solapan muchas
partículas el valor acumulado es alto y el mapeo lo lleva hacia el blanco, y en
los bordes se queda en rojo profundo. Pintando en 8 bits, todo se satura a
blanco en cuanto hay solapamiento.

Detalles asociados:

- **Elipse estirada verticalmente** según `|v_y|`: las partículas rápidas
  dibujan lenguas alargadas en vez de manchas redondas.
- **Estelas**: el campo no se borra entre frames, se multiplica por un factor
  de atenuación. Eso da el humo y el rastro de las chispas.
- **Emisión ∝ temp²**: aproxima que un cuerpo más caliente radia mucho más, y
  de paso permite descartar temprano las partículas ya apagadas.
- **Paleta de cuerpo negro** precalculada en 256 entradas por interpolación
  lineal entre siete puntos de control, de rescoldo a amarillo incandescente.

### 3.6 HUD sin dependencias extra

Los FPS se dibujan con una **fuente de mapa de bits 5x7 incluida en el propio
`renderer.cpp`** (7 bytes por glifo, un bit por punto). Añadir SDL_ttf habría
significado una dependencia más de instalación, un archivo de fuente que
distribuir y manejo de recursos adicional, todo para escribir dígitos y unas
pocas palabras en mayúsculas.

El contador es un **promedio sobre una ventana de 0.35 s**, no `1/dt`: es
estable de leer y sigue siendo lo bastante reactivo para notar caídas. Además se
imprime por consola una vez por segundo, lo que permite recoger mediciones
redirigiendo la salida a un archivo.

### 3.7 Optimizaciones del render secuencial

El primer prototipo funcionaba a **3 FPS**. Lo que lo llevó a ~41 FPS con los
valores por defecto, en orden de impacto medido:

| Cambio | Motivo |
|---|---|
| Curva gamma por tabla precalculada | el mapeo de tonos hacía 2.8 millones de `pow()` por frame; era el mayor coste fijo |
| Atenuación de estelas dentro del bucle del mapeo de tonos | elimina una pasada completa sobre el campo, ~2.8 M de escrituras |
| Tope al radio de brillo y umbral de emisión | las partículas frías dibujaban discos enormes que casi no aportaban luz |
| `-O3 -ffast-math` | permite vectorizar el bucle de acumulación |
| Recorrido por filas en el mapeo de tonos | el fondo se calcula una vez por fila, no por píxel |

Se probó también aproximar la gamma con una raíz cuadrada, esperando que el
compilador vectorizara el bucle; la medición mostró que era **más lento** que la
tabla (62 contra 81 FPS con `N=1`), así que se descartó. Las decisiones de
rendimiento de esta sección están tomadas sobre mediciones, no sobre intuición.

### 3.8 Programación defensiva

Toda la validación ocurre en `configParse`, **antes** de inicializar SDL o
reservar memoria. Se rechazan:

- valores no numéricos (`-n abc`), incluyendo casos parciales como `12abc`;
- opciones sin valor (`-n` como último argumento);
- opciones desconocidas;
- números fuera de rango, con el rango válido explícito en el mensaje;
- desbordamiento de `strtol`/`strtod` vía `errno`.

En cualquiera de esos casos el programa imprime el motivo, muestra el modo de
uso y termina con código 1 sin haber abierto ninguna ventana. Adicionalmente,
`main` captura `std::bad_alloc` para el caso de un `N` enorme que sí pase la
validación de rango pero no quepa en memoria, y el paso de tiempo del bucle se
acota entre 1/1000 s y 1/15 s para que un parón del sistema operativo no dispare
las partículas fuera de la pantalla.

### 3.9 Gestión de recursos

`Renderer` posee la ventana, el renderer y la textura de SDL, las libera en su
destructor y está marcada como no copiable. En `main`, el `Renderer` y el
`FireSystem` viven en un ámbito interno para garantizar que sus destructores
corran **antes** de `SDL_Quit()`, que es el orden correcto. No hay punteros
crudos con propiedad ni `new` fuera de los contenedores estándar.

---

## 4. Flujo de un frame

```
main
 ├─ handleEvents()            ESC / Q / cerrar ventana
 ├─ dt = reloj de alta resolución, acotado
 ├─ FireSystem::update(dt)    O(N): turbulencia, fuerzas, integración,
 │                            enfriamiento y reciclado de partículas muertas
 └─ Renderer::drawFrame()
     ├─ accumulateParticles() O(N · área): suma el brillo al campo RGB
     ├─ accumulateHearth()    halo del lecho de brasas
     ├─ toneMap()             O(W · H): campo -> ARGB, y atenúa las estelas
     ├─ drawLogs()            leños en primer plano
     ├─ drawHud()             FPS, N, resolución
     └─ SDL_UpdateTexture / RenderCopy / RenderPresent
```

---

## 5. Preparación para la versión paralela

La arquitectura deja identificados los dos puntos calientes, que son también los
dos patrones de descomposición del curso:

1. **`FireSystem::update`** — descomposición de datos pura. Cada partícula se
   actualiza sólo a partir de su propio estado; la única dependencia compartida
   es el PRNG usado en `respawn`, que habrá que resolver con un generador por
   hilo para no serializar ni introducir una condición de carrera.

2. **`Renderer::accumulateParticles`** — es el bucle más caro y tiene un
   conflicto real: dos partículas solapadas escriben en el mismo píxel del
   campo. Es el caso de libro para discutir reducción sobre el campo, campos
   privados por hilo, o partición del canvas en bandas horizontales.

3. **`Renderer::toneMap`** — descomposición trivial por filas, sin conflictos.

Las mediciones de la tabla del [README](fogata/README.md) sirven de línea base
para calcular speedup y eficiencia en la segunda entrega.
