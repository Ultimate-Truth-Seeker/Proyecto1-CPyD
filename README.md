# Arquitectura — Screensaver de fogata (versión secuencial y paralela)

Documento de diseño del Proyecto #1 de Computación Paralela y Distribuida: el
screensaver de una fogata nocturna, implementado en C++17 con SDL2. Incluye dos
versiones compilables: **secuencial** (código base) y **paralela** (con pragmas
de OpenMP). La base del código es idéntica en ambas; solo cambia la presencia
de directivas de paralelización durante la compilación.

El código vive en la carpeta `fogata/` del repositorio, separado del scaffolding
previo del simulador de galaxia, que queda intacto.

> **Sobre los comentarios en el código.** Por decisión explícita del equipo, el
> código fuente no lleva comentarios: la explicación de qué hace cada cosa y por
> qué está en este documento y en el [README](fogata/README.md), y el código se
> apoya en nombres descriptivos y funciones cortas. Conviene tener presente que
> la rúbrica del enunciado asigna puntos a los comentarios explicativos
> (requisito A y el 5% de "Documentación y comentarios").

---

## 1. Qué pide el enunciado y dónde se resuelve

| Requisito del enunciado | Dónde se cumple |
|---|---|
| Recibir al menos un parámetro `N` | `-n` en [config.cpp](fogata/src/config.cpp) |
| Varios colores pseudoaleatorios | tintes por partícula en `FireSystem::respawn`, PRNG `Rng` |
| Canvas mínimo de 640x480 | validado en `limits::kMinWidth/kMinHeight` |
| Movimiento y estética | partículas + campo de luz, bloom, cielo estrellado, luz sobre el suelo |
| Física o trigonometría | flotabilidad, arrastre, confinamiento, turbulencia por suma de senos |
| Mostrar FPS | HUD en pantalla y reporte por consola cada segundo |
| Programación defensiva | validación completa de argumentos antes de tocar SDL |
| Evitar variables hard-coded | todo lo ajustable vive en `struct Config` |

---

## 2. Estructura de archivos

```
Proyecto1-CPyD/
├── README.md                  <- este documento
└── fogata/
    ├── Makefile               compilación, detección de SDL2, flags de optimización
    ├── README.md              dependencias, parámetros, mediciones de FPS
    ├── docs/
    │   └── captura.png        captura del screensaver en ejecución
    ├── include/
    │   ├── config.h           struct Config + contrato del parseo de argumentos
    │   ├── particle.h         Rng, Particle, ParticleKind, FireSystem
    │   └── renderer.h         Star, Renderer
    └── src/
        ├── config.cpp         parseo y validación de la línea de comandos
        ├── particle.cpp       modelo físico del fuego
        ├── renderer.cpp       render por software, escena nocturna y HUD
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
  sobre un arreglo de partículas.
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
- De cara a OpenMP: el arreglo tiene tamaño conocido y estable, así que el bucle
  de física es un `for` de índice entero, el caso más simple de repartir.

### 3.3 La temperatura hace de variable de vida

No hay campo `vida` ni `edad`. La temperatura normalizada `temp` cumple tres
funciones a la vez: controla la fuerza de flotabilidad, elige el color en la
paleta de cuerpo negro y determina cuándo la partícula muere. Un solo escalar en
lugar de tres, y el resultado es físicamente coherente: lo que se apaga es lo
que se ha enfriado.

### 3.4 Tres clases de partícula, un solo arreglo

`ParticleKind` distingue **llama**, **chispa** y **humo**. Las tres viven en el
mismo `std::vector` y recorren el mismo bucle; sólo cambian sus constantes
(arrastre, enfriamiento, radio, color, empuje). Alternativas descartadas:

- tres arreglos separados → tres bucles, tres reparticiones OpenMP, más código;
- jerarquía de clases con métodos virtuales → una llamada indirecta por
  partícula y por frame, imposible de vectorizar.

Con un `enum` en un `struct` plano el bucle sigue siendo un recorrido lineal
sobre memoria contigua.

### 3.5 Modelo físico

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
   El humo lleva el confinamiento casi anulado y el viento multiplicado, que es
   lo que hace que la columna se despegue y se deshaga en volutas.

3. **Integración de Euler semi-implícita** (primero velocidad, luego posición).
   Es estable para este rango de fuerzas, cuesta O(N) y no requiere guardar
   estado del paso anterior.

Las ráfagas de viento son tres senos de períodos inconmensurables, de modo que
el patrón nunca se repite de forma evidente. El parpadeo global de la fogata
(`FireSystem::flicker`) se construye igual, con tres senos de frecuencias
distintas, y es lo que hace latir la luz sobre el suelo, los leños y las
piedras: todos los elementos de la escena laten en fase porque todos leen el
mismo escalar.

### 3.6 Render: campo de luz en coma flotante

La decisión que define el aspecto del programa. En lugar de pintar cada
partícula directamente sobre el framebuffer de 8 bits:

1. Cada partícula **suma** su brillo (color de cuerpo negro × tinte × emisión) a
   un campo RGB de `float`, con caída suave `(1 - d²)²` sobre una elipse.
2. Sólo al final el campo completo pasa por un mapeo de tonos.

Sumar en coma flotante y comprimir el rango una única vez es lo que produce el
núcleo incandescente rodeado de naranja: donde se solapan muchas partículas el
valor acumulado es alto y el mapeo lo lleva hacia el blanco, y en los bordes se
queda en rojo profundo. Pintando en 8 bits, todo se satura a blanco en cuanto
hay solapamiento.

Detalles asociados:

- **Elipse estirada verticalmente** según `|v_y|`: las partículas rápidas
  dibujan lenguas alargadas en vez de manchas redondas.
- **Estelas**: el campo no se borra entre frames, se multiplica por un factor de
  atenuación. Eso da el humo y el rastro de las chispas.
- **Emisión ∝ temp²**: aproxima que un cuerpo más caliente radia mucho más, y de
  paso permite descartar temprano las partículas ya apagadas.
- **Base azul**: las partículas muy calientes y muy cercanas al lecho mezclan su
  color hacia el azul, como el cono de combustión completa de un fuego real.
- **Normalización por densidad**: la emisión se divide por `N / N_referencia`,
  así subir `N` añade detalle en lugar de saturar la pantalla a blanco.

### 3.7 La escena: lo que se calcula una sola vez

Lo que convierte "unas partículas naranjas sobre negro" en una fogata de noche
son tres capas que **no dependen del tiempo** y por lo tanto se precalculan en
`Renderer::init`:

| Capa | Qué es | Coste por frame |
|---|---|---|
| `background_` | degradado del cielo, suelo con grano y viñeteado | una lectura |
| `firelight_` | cuánto ilumina la fogata cada píxel (charco en el suelo + halo en el aire) | una lectura × un escalar |
| `stars_` | lista de estrellas con brillo y fase propios | ~500 sumas |

`firelight_` es la idea central de la escena: en vez de recalcular cada frame la
luz que la fogata derrama sobre el suelo, se guarda **un flotante por píxel** y
en tiempo de ejecución sólo se multiplica por el parpadeo. Toda la iluminación
ambiental de la escena late con el fuego a cambio de una multiplicación por
píxel. El horizonte se mezcla con una banda suave, tanto en el fondo como en la
luz, porque un corte duro entre cielo y suelo se nota inmediatamente.

Las estrellas sí se suman al campo cada frame, con un centelleo senoidal por
estrella; al entrar en el campo, las recoge el bloom y quedan con un halo suave
en lugar de ser píxeles duros.

### 3.8 Bloom: el desenfoque que hace que la luz "queme"

El brillo difuso alrededor de la llama y de las chispas se obtiene reduciendo el
campo a un cuarto de resolución, desenfocándolo con dos pasadas de caja
(horizontal y vertical, con suma deslizante, O(1) por píxel) y sumándolo de
vuelta atenuado.

La sutileza está en **cuándo** se hace la reducción. La versión obvia recorre el
campo entero una vez más para reducirlo, y ese recorrido (casi 3 millones de
lecturas) costaba más que el desenfoque en sí. En la versión final la reducción
se acumula **dentro del bucle de composición**, que ya está leyendo cada píxel
del campo, y el desenfoque se aplica después: el bloom que se ve en pantalla es
el del frame anterior. Un frame de retraso en un halo difuso es imperceptible, y
ahorra una pasada completa sobre 11 MB.

### 3.9 HUD sin dependencias extra

Los FPS se dibujan con una **fuente de mapa de bits 5x7 incluida en el propio
`renderer.cpp`** (7 bytes por glifo, un bit por punto). Añadir SDL_ttf habría
significado una dependencia más de instalación, un archivo de fuente que
distribuir y manejo de recursos adicional, todo para escribir dígitos y unas
pocas palabras en mayúsculas.

El contador es un **promedio sobre una ventana de 0.35 s**, no `1/dt`: es
estable de leer y sigue siendo lo bastante reactivo para notar caídas. Además se
imprime por consola una vez por segundo, lo que permite recoger mediciones
redirigiendo la salida a un archivo.

### 3.10 Optimizaciones del render secuencial

El primer prototipo funcionaba a **3 FPS**. La versión actual, con una escena
bastante más cara, corre a ~47 FPS con los valores por defecto. En orden de
impacto medido:

| Cambio | Motivo |
|---|---|
| `-march=native` | ~45% más de FPS: permite vectorizar los bucles del render con el juego de instrucciones de la máquina donde se compila |
| Curva gamma por tabla precalculada | el mapeo de tonos hacía 2.8 millones de `pow()` por frame; era el mayor coste fijo |
| Reducción del bloom fusionada en la composición | elimina una pasada completa sobre el campo (11 MB) |
| Atenuación de estelas dentro del mismo bucle | elimina otra pasada completa |
| Tope al radio de brillo y umbral de emisión | las partículas frías dibujaban discos enormes que casi no aportaban luz |
| Iluminación de la escena precalculada | el halo y el charco de luz eran un bucle por frame; ahora son una tabla |
| `SDL_LockTexture` en vez de `SDL_UpdateTexture` | se escribe directamente en la textura, sin copia intermedia de 3.7 MB |
| Recorrido por filas en la composición | el fondo se calcula una vez por fila, no por píxel |

Se probó también aproximar la gamma con una raíz cuadrada, esperando que el
compilador vectorizara el bucle; la medición mostró que era **más lento** que la
tabla (62 contra 81 FPS con `N=1`), así que se descartó. Las decisiones de
rendimiento de esta sección están tomadas sobre mediciones, no sobre intuición.

### 3.11 Programación defensiva

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

### 3.12 Gestión de recursos

`Renderer` posee la ventana, el renderer y la textura de SDL, las libera en su
destructor y está marcada como no copiable. En `main`, el `Renderer` y el
`FireSystem` viven en un ámbito interno para garantizar que sus destructores
corran **antes** de `SDL_Quit()`, que es el orden correcto. La textura se bloquea
y se desbloquea dentro del mismo frame, y si el bloqueo falla el frame se salta
en lugar de escribir en un puntero inválido. No hay punteros crudos con
propiedad ni `new` fuera de los contenedores estándar.

### 3.13 Sistema dual-mode: secuencial y paralelo en un mismo código

Para facilitar la comparación de rendimiento, el código soporta compilación
en dos modos:

**Modo secuencial** (por defecto):
- Se compila sin `-fopenmp`
- Los pragmas de `#pragma omp parallel for` son ignorados por el compilador
- El código se ejecuta en un único hilo
- Se genera en `build-sequential/`

**Modo paralelo**:
- Se compila con `-fopenmp`
- Los pragmas se reconocen y se generan bucles multihilo
- Se ejecuta en todos los cores disponibles
- Se genera en `build-parallel/`

El **Makefile** controla qué modo se usa mediante la variable `BUILD_TYPE`:
```makefile
make sequential    # Compila versión secuencial
make parallel      # Compila versión paralela
```

La ventaja de este enfoque es:

1. **Código únicamente, sin cambios de lógica**: los pragmas son directivas
   indiferentes para el compilador en modo secuencial. No hay código condicional
   tipo `#ifdef PARALLEL`.

2. **Medición limpia**: ambos binarios tienen el mismo layout de memoria, las
   mismas optimizaciones `-O3 -ffast-math -march=native`, solo cambia si se
   reconocen los pragmas de OpenMP.

3. **Debugging facilitado**: se puede correr la versión secuencial bajo `gdb`
   o `valgrind` sin introducir hilos.

El precio es reservar un poco más de espacio en disco (dos ejecutables y dos
directorios de compilación).

---

## 4. Flujo de un frame

```
main
 ├─ handleEvents()             ESC / Q / cerrar ventana
 ├─ dt = reloj de alta resolución, acotado
 ├─ FireSystem::update(dt)     O(N): turbulencia, fuerzas, integración,
 │                             enfriamiento y reciclado de partículas muertas
 └─ Renderer::drawFrame()
     ├─ accumulateStars()      centelleo de las estrellas
     ├─ accumulateParticles()  O(N · área): suma el brillo al campo RGB
     ├─ SDL_LockTexture()
     │   ├─ composite()        O(W·H): campo + fondo + luz de fogata + bloom
     │   │                     -> ARGB; atenúa la estela y reduce para el bloom
     │   ├─ drawLogs()         leños con brasas
     │   ├─ drawStones()       círculo de piedras iluminado por el fuego
     │   └─ drawHud()          FPS, N, resolución
     ├─ SDL_UnlockTexture()
     ├─ blurBloom()            desenfoque de caja para el frame siguiente
     └─ SDL_RenderCopy / RenderPresent
```

---

## 5. Preparación para la versión paralela

La arquitectura deja identificados los puntos calientes, que son también los
patrones de descomposición del curso:

1. **`FireSystem::update`** — descomposición de datos pura. Cada partícula se
   actualiza sólo a partir de su propio estado; la única dependencia compartida
   es el PRNG usado en `respawn`, que habrá que resolver con un generador por
   hilo para no serializar ni introducir una condición de carrera.

2. **`Renderer::accumulateParticles`** — es el bucle más caro y tiene un
   conflicto real: dos partículas solapadas escriben en el mismo píxel del
   campo. Es el caso de libro para discutir reducción sobre el campo, campos
   privados por hilo, o partición del canvas en bandas horizontales.

3. **`Renderer::composite`** — descomposición trivial por filas, sin conflictos,
   salvo por el acumulador del bloom, que se comparte entre las filas de un
   mismo bloque: o se reparte por bloques de bloom en lugar de por filas, o cada
   hilo acumula en su propio bloque.

4. **`Renderer::blurBloom`** — las dos pasadas son independientes por fila y por
   columna respectivamente; la barrera entre ambas es obligatoria.

Las mediciones de la tabla del [README](fogata/README.md) sirven de línea base
para calcular speedup y eficiencia en la segunda entrega.

---

## 6. Pragmas de OpenMP utilizados

### 6.1 FireSystem::update() — `particle.cpp` línea ~107

```cpp
#pragma omp parallel for schedule(dynamic, 64)
for (int i = 0; i < static_cast<int>(particles_.size()); ++i) {
    Particle& p = particles_[i];
    // Actualización física de cada partícula
}
```

**Por qué funciona**: cada partícula es completamente independiente. No hay
lecturas/escrituras compartidas salvo el PRNG, que será tratado después.

**Estrategia de scheduling**: `dynamic` con chunk size 64. Las partículas no
tienen carga uniforme (chispas vs humo), así que la distribución estática sería
desbalanceada. El chunk size de 64 es un compromiso entre overhead de
sincronización y balance de carga.

### 6.2 Renderer::accumulateParticles() — `renderer.cpp` línea ~287

```cpp
#pragma omp parallel for schedule(dynamic, 64)
for (int pi = 0; pi < static_cast<int>(fire.particles().size()); ++pi) {
    const Particle& p = fire.particles()[pi];
    // Suma el brillo de cada partícula al campo flotante
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            row[c] += colorR * weight;  // <-- ESCRIBTURA A MEMORIA COMPARTIDA
        }
    }
}
```

**Conflicto real**: múltiples partículas pueden solaparseel brillo en el mismo
píxel. Esto es un **data race**. La solución será (en una entrega posterior)
uno de:
- Campos privados por hilo y reducción
- Partición del canvas en bandas
- Actualización atómica (lenta)

Por ahora, el pragma se documenta como "experimental".

### 6.3 Renderer::composite() — `renderer.cpp` línea ~349

```cpp
#pragma omp parallel for
for (int y = 0; y < height_; ++y) {
    // Composición del campo RGB, fondo, y luz
    // Cada hilo escribe filas distintas: sin conflictos
}
```

**Por qué funciona**: cada fila se escribe por un único hilo, sin solapamiento.

**Detalle**: el acumulador `raw` del bloom se escribe una vez por bloque de
`kBloomScale` filas. Esto requiere sincronización, que queda cargo del usuario
(un `#pragma omp barrier` implícito al fin del loop).

### 6.4 Renderer::blurBloom() — `renderer.cpp` líneas ~397 y ~419

Dos pasadas independientes, cada una paralelizable:

```cpp
// Pasada horizontal
#pragma omp parallel for
for (int y = 0; y < bloomHeight_; ++y) { /* sumar deslizante en x */ }

// [Barrera implícita aquí]

// Pasada vertical
#pragma omp parallel for
for (int x = 0; x < bloomWidth_; ++x) { /* sumar deslizante en y */ }
```

**Estructura**: dos bucles independientes paralelizables, cada uno con lectura
de un arreglo y escritura a otro. Sin conflictos dentro de cada pasada. La
barrera entre ambas es implícita: el fin del primer pragma y el comienzo del
segundo.

---

## 5. Preparación para la versión paralela

La arquitectura deja identificados los puntos calientes, que son también los
patrones de descomposición del curso:

1. **`FireSystem::update`** — descomposición de datos pura. Cada partícula se
   actualiza sólo a partir de su propio estado; la única dependencia compartida
   es el PRNG usado en `respawn`, que habrá que resolver con un generador por
   hilo para no serializar ni introducir una condición de carrera.

2. **`Renderer::accumulateParticles`** — es el bucle más caro y tiene un
   conflicto real: dos partículas solapadas escriben en el mismo píxel del
   campo. Es el caso de libro para discutir reducción sobre el campo, campos
   privados por hilo, o partición del canvas en bandas horizontales.

3. **`Renderer::composite`** — descomposición trivial por filas, sin conflictos,
   salvo por el acumulador del bloom, que se comparte entre las filas de un
   mismo bloque: o se reparte por bloques de bloom en lugar de por filas, o cada
   hilo acumula en su propio bloque.

4. **`Renderer::blurBloom`** — las dos pasadas son independientes por fila y por
   columna respectivamente; la barrera entre ambas es obligatoria.
