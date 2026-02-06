// TODO: Check game over
// TODO: Break / Split asteroids

#include "raylib.h"
#include "raymath.h"

#include <math.h>
#include <stdlib.h>

#define WIDTH 800
#define HEIGHT 600

#define NUM_ASTEROIDS 6
#define SCALE 0.8f
#define VEL 2.0f
#define BULLET_SPEED 5.0f
#define NUM_BULLETS 10

typedef struct {
    Vector2 pos;
    int sides;
    float radius, rotation;
    Vector2 vel;
    float mass;
} Asteroid;

typedef struct {
    Vector2 pos;
    float radius;
    Vector2 vel; // TODO: This is not used. Figure out what to do with this
} Spaceship;

typedef struct {
    Vector2 pos;
    float radius;
    Vector2 vel;
} Bullet;

Asteroid ASTEROIDS[NUM_ASTEROIDS];
Bullet BULLETS[NUM_BULLETS];
int bulletActive[NUM_BULLETS];

Vector2 getRandV() {
    /**
    * Returns a random unit vector (random direction)
    */

    float angle = (float)GetRandomValue(0, 369) * DEG2RAD;
    return (Vector2){cosf(angle), sinf(angle)};
}

void initAsteroids() {
    /** 
    * Initialize asteroids with random positions/sizes/sides and random directions
    */

    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        Vector2 pos;
        int sides = GetRandomValue(3, 8);
        float radius = GetRandomValue(35, 65);
        float rotation = GetRandomValue(1, 5);
        pos.x = GetRandomValue(0 + radius, WIDTH - radius);
        pos.y = GetRandomValue(0 + radius, HEIGHT - radius);
        float mass = radius * radius;
        ASTEROIDS[i] = (Asteroid){pos, sides, radius, rotation, getRandV(), mass};
    }
}

void resolveCollisions(Asteroid *a, Asteroid *b) {
    /**
    * Resolve an asteroid-asteroid collision as a perfectly elastic collision.
    * Steps:
    * 1) Early-out if not overlapping
    * 2) Compute unit normal/tangent at contact
    * 3) Separate positions to remove overlap (positional correction)
    * 4) Project velocities into normal/tangent components
    * 5) Apply 1D elastic collision equations along the normal; tangential components remain unchanged. 
    * 6) Reconstruct final velocity vectors
    */

    Vector2 delta = Vector2Subtract(a->pos, b->pos);
    float dsq = Vector2LengthSqr(delta);
    float rsum = a->radius + b->radius;

    if (dsq <= 0.0000001f || dsq >= rsum * rsum)
        return;

    float dist = sqrtf(dsq);
    Vector2 unitNormal = Vector2Scale(delta, 1.0f / dist);
    Vector2 unitTangent = (Vector2){-unitNormal.y, unitNormal.x};

    Vector2 rv = Vector2Subtract(a->vel, b->vel);
    float velAlongNormal = Vector2DotProduct(rv, unitNormal);
    if (velAlongNormal > 0.0f)
        return;

    // Positional correction
    float overlap = rsum - dist;
    float invMa = (a->mass > 0) ? 1.0f / a->mass : 0.0f;
    float invMb = (b->mass > 0) ? 1.0f / b->mass : 0.0f;
    float invSum = invMa + invMb;
    if (invSum > 0.0f) {
        Vector2 correction = Vector2Scale(unitNormal, overlap / invSum);
        a->pos = Vector2Add(a->pos, Vector2Scale(correction, invMa));
        b->pos = Vector2Subtract(b->pos, Vector2Scale(correction, invMb));
    }

    // Decompose velocities into normal (n) and tangential (t) scalar components
    float va_n = Vector2DotProduct(a->vel, unitNormal);
    float va_t = Vector2DotProduct(a->vel, unitTangent);
    float vb_n = Vector2DotProduct(b->vel, unitNormal);
    float vb_t = Vector2DotProduct(b->vel, unitTangent);

    // Elastic collision along the normal axis (1D).
    float va_np = (va_n * (a->mass - b->mass) + 2.0f * b->mass * vb_n) / (a->mass + b->mass);
    float vb_np = (vb_n * (b->mass - a->mass) + 2.0f * a->mass * va_n) / (a->mass + b->mass);

    // Recompose final velocities: v' = v_n' * n + v_t * t (tangential unchanged)
    Vector2 va_np_vector = Vector2Scale(unitNormal, va_np);
    Vector2 va_tp_vector = Vector2Scale(unitTangent, va_t);
    Vector2 vb_np_vector = Vector2Scale(unitNormal, vb_np);
    Vector2 vb_tp_vector = Vector2Scale(unitTangent, vb_t);

    Vector2 velFinal_A = Vector2Add(va_np_vector, va_tp_vector);
    Vector2 velFinal_B = Vector2Add(vb_np_vector, vb_tp_vector);

    a->vel = velFinal_A;
    b->vel = velFinal_B;
}

void checkCollisions() {
    /**
    * Check and resolve collisions for every unique asteroid pair
    */

    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        for (int j = i + 1; j < NUM_ASTEROIDS; j++) {
            Asteroid *a = &ASTEROIDS[i];
            Asteroid *b = &ASTEROIDS[j];

            resolveCollisions(a, b);
        }
    }
}

void MoveSpaceship(Spaceship *s) {
    /**
    * Ship movement with keyboard (direct position changes)
    */

    if (IsKeyDown(KEY_RIGHT))
        s->pos.x += VEL;
    if (IsKeyDown(KEY_LEFT))
        s->pos.x -= VEL;
    if (IsKeyDown(KEY_UP))
        s->pos.y -= VEL;
    if (IsKeyDown(KEY_DOWN))
        s->pos.y += VEL;
}

void UpdateSpaceship(Spaceship *s) {
    /**
    *Update ship state
    */

    MoveSpaceship(s);
    s->pos.x += s->vel.x * VEL;
    s->pos.y += s->vel.y * VEL;
}

// TODO: Shooting
// Mecánica de disparo:
// 1. Obtener la posición actual de la nave espacial
// 2. Comprobar si el jugador presiona la barra espaciadora
// 3. Dibujar la (1) bala (DrawCircle) desde el punto (no sé cuál) de la nave espacial
// 4. Mover la bala con una dirección y velocidad
// 5. Si la bala sale de la pantalla o golpea un asteroide, eliminarla.
//
// Funciones:
// - createBullet(): comprueba si la entrada del usuario = KEY_SPACE, crea una bala y la añade al
// array si es posible
// - drawBullets(): dibuja cuántas balas hay en el array
// - moveBullet(Bullet* b): mueve la bala con dirección de velocidad y magnitud (velocidad)
// - checkBulletBounds(Bullet* b): recorre el array de balas y comprueba si está fuera de los
// límites o si golpea un asteroide
// - Envolver todo en el método shoot(Spaceship* s) para que sea más limpio

// Función para comprobar espacio libre en el array de balas?

// Si no hay espacio libre, eliminar la primera (FIFO)
// Si hay espacio libre, crear bala

void createBullet(Vector2 pos, Vector2 velDir) {
    /**
    * Creates a bullet at the given position and direction if SPACE is pressed.
    */

    if (!IsKeyPressed(KEY_SPACE))
        return;

    for (int i = 0; i < NUM_BULLETS; i++) {
        if (!bulletActive[i]) {
            BULLETS[i] = (Bullet){pos, 3, velDir};
            bulletActive[i] = 1;
            return;
        }
    }

    // TODO: Implementar FIFO con queue
    // Esto funciona por ahora
    BULLETS[0] = (Bullet){pos, 3, velDir};
    bulletActive[0] = 1;
}

void drawBullets() {
    for (int i = 0; i < NUM_BULLETS; i++) {
        if (!bulletActive[i])
            continue;
        Bullet *b = &BULLETS[i];
        DrawCircle(b->pos.x, b->pos.y, b->radius, RAYWHITE);
    }
}

int bulletHit() {
    /**
    * Checks for collisions between bullets and asteroids.
    */

    for (int i = 0; i < NUM_BULLETS; i++) {
        if (!bulletActive[i])
            continue;
        Bullet *b = &BULLETS[i];

        for (int j = 0; j < NUM_ASTEROIDS; j++) {
            Asteroid *a = &ASTEROIDS[j];

            float dx = b->pos.x - a->pos.x;
            float dy = b->pos.y - a->pos.y;
            float r = b->radius + a->radius;

            if (dx*dx + dy*dy <= r*r) {
                return 1;
            }
        }
    }
    return 0;
}

void updateBullets() {
    /**
    * Deactivates bullets that go off-screen or hit an asteroid.
    */

    for (int i = 0; i < NUM_BULLETS; i++) {
        if (!bulletActive[i])
            continue;
        Bullet *b = &BULLETS[i];

        if (b->pos.x + b->radius < 0 || b->pos.x - b->radius > WIDTH || b->pos.y + b->radius < 0 ||
            b->pos.y - b->radius > HEIGHT || bulletHit()) {
            bulletActive[i] = 0;
        }
    }
}

void moveBullet() {
    /**
    * Moves all active bullets according to their velocity.
    */

    for (int i = 0; i < NUM_BULLETS; i++) {
        if (!bulletActive[i])
            continue;
        Bullet *b = &BULLETS[i];
        b->pos.x += b->vel.x * BULLET_SPEED;
        b->pos.y += b->vel.y * BULLET_SPEED;
    }
}

void Shoot(Spaceship *s) {
    /**
    * Handles bullet creation, update, movement, and rendering.
    */

    Vector2 position = s->pos;
    updateBullets();
    createBullet(position, (Vector2){1, 0});
    moveBullet();
    drawBullets();
}

void UpdateAsteroids() {
    /**
    * Update asteroid positions, screen wrap-around, and visual rotation
    */

    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        Asteroid *a = &ASTEROIDS[i];

        a->pos.x += a->vel.x * SCALE;
        a->pos.y += a->vel.y * SCALE;

        if (a->pos.x + a->radius < 0)
            a->pos.x = WIDTH;
        if (a->pos.x - a->radius > WIDTH)
            a->pos.x = 0;
        if (a->pos.y + a->radius < 0)
            a->pos.y = HEIGHT;
        if (a->pos.y - a->radius > HEIGHT)
            a->pos.y = 0;

        a->rotation += SCALE;
    }
}

void UpdateGame(Spaceship *s) {
    UpdateAsteroids();
    UpdateSpaceship(s);
    checkCollisions();
}

void DrawSpaceShip(Spaceship *s) { DrawPoly(s->pos, 3, s->radius, 120, BLUE); }

void DrawAsteroids() {
    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        Asteroid *a = &ASTEROIDS[i];
        DrawPoly(a->pos, a->sides, a->radius, a->rotation, RAYWHITE);
    }
}

int main(void) {
    InitWindow(WIDTH, HEIGHT, "Asteroid");
    SetTargetFPS(60);

    initAsteroids();
    Spaceship spaceship =
        (Spaceship){(Vector2){(float)WIDTH / 2, (float)HEIGHT / 2}, 10, (Vector2){0, 0}};

    int gameOver = 0; // TODO: Use this (set when ship hits asteroid, etc.)
    (void)gameOver;   // Silence warning until implemented

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        UpdateGame(&spaceship);
        DrawAsteroids();
        DrawSpaceShip(&spaceship);
        Shoot(&spaceship);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
