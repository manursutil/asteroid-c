// TODO: Check game over 
// TODO: Shooting
// TODO: Break / Split asteroids
// TODO: Add screen wrap instead of bouncing

#include "raylib.h"
#include "raymath.h"

#include <math.h>
#include <stdlib.h>

#define WIDTH 800
#define HEIGHT 600

#define NUM_ASTEROIDS 6
#define SCALE 0.8f
#define VEL 2.0f

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

Asteroid ASTEROIDS[NUM_ASTEROIDS];

Vector2 getRandV() {
    float angle = (float)GetRandomValue(0, 369) * DEG2RAD;
	return (Vector2){cosf(angle), sinf(angle)};
}

void initAsteroids() {
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

// TODO: Finish this
void resolveCollisions(Asteroid* a, Asteroid* b) {
    // Find unit normal and unit tangent vectors
    Vector2 delta = Vector2Subtract(a->pos, b->pos);
    float dsq = Vector2LengthSqr(delta);
    float rsum = a->radius + b->radius;

    if (dsq <= 0.0000001f || dsq >= rsum*rsum) return;

    float dist = sqrtf(dsq);
    Vector2 unitNormal = Vector2Scale(delta, 1.0f/dist);
    Vector2 unitTangent = (Vector2){-unitNormal.y, unitNormal.x};
    
    // Create initial (before the collision velovty vector, v1 and v2) -- Already done
    // Resolve the velocity vectors (v1 and v2) into normal and angential components. Project the velocity vectors ontro the unit normal and the unit tangent vectors by taking the dot product of the vlocity vectors with the normal and unit tangent vectors. Vn and Vt are scalars.
    float velANormal = Vector2DotProduct(a->vel, unitNormal);
    float velATangent = Vector2DotProduct(a->vel, unitTangent);
    float velBNormal = Vector2DotProduct(b->vel, unitNormal);
    float velBTangent = Vector2DotProduct(b->vel, unitTangent);

    // Find the new tangential velocities (after the collision). They do not change: v1_t' = v1_t, v2_t' = v2_t.
    // Find the new normal velocities (after the collision ("prime")). Same as one-dimensional collision formulas:
    // v1_n' = v1_n * (m1 - m2) + 2*m2*v2_n / m1 + m2
    // v2_n' = v2_n * (m1 - m2) + 2*m1*v1_n / m1 + m2

    // Convert the scalar normal and tangential velocities into vectors. Multiply the unit normal vector by the scalar normal velocity, same for the tangential component

    // Find the final volicyt vectors by adding the normal and tangential components for each object:
    // v1' = v1_n' + v1_t'
    // v2' = v2_n' + v2_t'
}

void checkCollisions() {
    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        for (int j = i + 1; j < NUM_ASTEROIDS; j++) {
            Asteroid *a = &ASTEROIDS[i];
            Asteroid *b = &ASTEROIDS[j];

            float dx = a->pos.x - b->pos.x;
            float dy = a->pos.y - b->pos.y;
            float dsq = dx*dx + dy*dy;
            float radiusSum = a->radius + b->radius;

            // TODO: Better collision handling
            if (dsq < radiusSum * radiusSum) {
                a->vel.x *= -1;
                a->vel.y *= -1;
                b->vel.x *= -1;
                b->vel.x *= -1;
            }
        }
    }
}

void MoveSpaceship(Spaceship* s) {
	if (IsKeyDown(KEY_RIGHT)) s->pos.x += VEL;
    if (IsKeyDown(KEY_LEFT)) s->pos.x -= VEL;
    if (IsKeyDown(KEY_UP)) s->pos.y -= VEL;
    if (IsKeyDown(KEY_DOWN)) s->pos.y += VEL;
}

void UpdateSpaceship(Spaceship* s) {
    MoveSpaceship(s);
    s->pos.x += s->vel.x * VEL;
    s->pos.y += s->vel.y * VEL;
}

void UpdateAsteroids() {
    for (int i = 0; i < NUM_ASTEROIDS; i++) {
        Asteroid *a = &ASTEROIDS[i];
		
		a->pos.x += a->vel.x * SCALE;
		a->pos.y += a->vel.y * SCALE;

        if (a->pos.x - a->radius < 0 || a->pos.x + a->radius > WIDTH) {
            a->vel.x *= -1;
        } else if (a->pos.y - a->radius < 0 || a->pos.y + a->radius > HEIGHT) {
            a->vel.y *= -1;
        }

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

    int gameOver = 0;

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