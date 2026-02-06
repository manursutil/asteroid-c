// TODO: Check game over
// TODO: Shooting
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
    Vector2 vel;
} Spaceship;

typedef struct {
    Vector2 pos;
    float radius;
    Vector2 vel;
} Bullet;

Asteroid ASTEROIDS[NUM_ASTEROIDS];
Bullet BULLETS[NUM_BULLETS];

Vector2 getRandV() {
    /* Returns a random unit vector (random direction). */

    float angle = (float)GetRandomValue(0, 369) * DEG2RAD;
	return (Vector2){cosf(angle), sinf(angle)};
}

void initAsteroids() {
    /* Initialize asteroids with random positions/sizes/sides and random directions. */

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

void resolveCollisions(Asteroid* a, Asteroid* b) {
/*
    Resolve an asteroid-asteroid collision as a perfectly elastic collision.
    Steps:
    1) Early-out if not overlapping.
    2) Compute unit normal/tangent at contact.
    3) Separate positions to remove overlap (positional correction).
    4) Project velocities into normal/tangent components.
    5) Apply 1D elastic collision equations along the normal; tangential components remain unchanged.
    6) Reconstruct final velocity vectors.
*/

    Vector2 delta = Vector2Subtract(a->pos, b->pos);
    float dsq = Vector2LengthSqr(delta);
    float rsum = a->radius + b->radius;

    if (dsq <= 0.0000001f || dsq >= rsum*rsum) return;

    float dist = sqrtf(dsq);
    Vector2 unitNormal = Vector2Scale(delta, 1.0f/dist);
    Vector2 unitTangent = (Vector2){-unitNormal.y, unitNormal.x};

    Vector2 rv = Vector2Subtract(a->vel, b->vel);
    float velAlongNormal = Vector2DotProduct(rv, unitNormal);
    if (velAlongNormal > 0.0f) return;

    // Positional correction
    float overlap = rsum - dist;
    float invMa = (a->mass > 0) ? 1.0f/a->mass : 0.0f;
    float invMb = (b->mass > 0) ? 1.0f/b->mass : 0.0f;
    float invSum = invMa + invMb;
    if (invSum > 0.0f) {
        Vector2 correction = Vector2Scale(unitNormal, overlap / invSum);
        a->pos = Vector2Add(a->pos, Vector2Scale(correction, invMa));
        b->pos = Vector2Subtract(b->pos, Vector2Scale(correction, invMb));
    }

    // Decompose velocities into normal (n) and tangential (t) scalar components.
    float va_n = Vector2DotProduct(a->vel, unitNormal);
    float va_t = Vector2DotProduct(a->vel, unitTangent);
    float vb_n = Vector2DotProduct(b->vel, unitNormal);
    float vb_t = Vector2DotProduct(b->vel, unitTangent);

    // Elastic collision along the normal axis (1D).
    float va_np = (va_n * (a->mass - b->mass) + 2.0f*b->mass*vb_n) / (a->mass + b->mass);
    float vb_np = (vb_n * (b->mass - a->mass) + 2.0f*a->mass*va_n) / (a->mass + b->mass);

    // Recompose final velocities: v' = v_n' * n + v_t * t (tangential unchanged).
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
    /* Check and resolve collisions for every unique asteroid pair. */

    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        for (int j = i + 1; j < NUM_ASTEROIDS; j++) {
            Asteroid *a = &ASTEROIDS[i];
            Asteroid *b = &ASTEROIDS[j];

            resolveCollisions(a, b);
        }
    }
}

void MoveSpaceship(Spaceship* s) {
    /* Keyboard-driven ship movement (direct position changes). */

	if (IsKeyDown(KEY_RIGHT)) s->pos.x += VEL;
    if (IsKeyDown(KEY_LEFT)) s->pos.x -= VEL;
    if (IsKeyDown(KEY_UP)) s->pos.y -= VEL;
    if (IsKeyDown(KEY_DOWN)) s->pos.y += VEL;
}

void UpdateSpaceship(Spaceship* s) {
    /* Update ship state */

    MoveSpaceship(s);
    s->pos.x += s->vel.x * VEL;
    s->pos.y += s->vel.y * VEL;
}

void UpdateAsteroids() {
    /* Update asteroid positions, screen wrap-around, and visual rotation. */

    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        Asteroid *a = &ASTEROIDS[i];

		a->pos.x += a->vel.x * SCALE;
		a->pos.y += a->vel.y * SCALE;

        if (a->pos.x + a->radius < 0) a->pos.x = WIDTH;
        if (a->pos.x - a->radius > WIDTH) a->pos.x = 0;
        if (a->pos.y + a->radius < 0) a->pos.y = HEIGHT;
        if (a->pos.y - a->radius > HEIGHT) a->pos.y = 0;

        a->rotation += SCALE;
    }
}

void UpdateGame(Spaceship* s) {
	UpdateAsteroids();
	UpdateSpaceship(s);
	checkCollisions();
}

void DrawSpaceShip(Spaceship* s) {
    DrawPoly(s->pos, 3, s->radius, 120, BLUE);
}

void DrawAsteroids() {
    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        Asteroid *a = &ASTEROIDS[i];
        DrawPoly(a->pos, a->sides, a->radius, a->rotation, RAYWHITE);
    }
}

int main(void)
{
    InitWindow(WIDTH, HEIGHT, "Asteroid");
    SetTargetFPS(60);

    initAsteroids();
    Spaceship spaceship = (Spaceship) {
        (Vector2) {(float)WIDTH/2, (float)HEIGHT/2},
        10,
        (Vector2) {0,0}
    };

    int gameOver = 0; // TODO: Use this (set when ship hits asteroid, etc.)
    (void)gameOver; // Silence warning until implemented

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(BLACK);
			UpdateGame(&spaceship);
            DrawAsteroids();
            DrawSpaceShip(&spaceship);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
