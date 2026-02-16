// TODO: Refactorizar en diferentes módulos

#include "raylib.h"
#include "raymath.h"

#include <math.h>
#include <stdlib.h>

#define WIDTH 800
#define HEIGHT 600

#define MAX_ASTEROIDS 64
#define NUM_START_ASTEROIDS 6

#define R_BIG 55.0f
#define R_MED 35.0f
#define R_SMALL 18.0f

#define SCALE 0.8f
#define VEL 2.0f
#define BULLET_SPEED 5.0f
#define NUM_BULLETS 10

typedef enum {
    AST_SMALL,
    AST_MED,
    AST_BIG,
} AsteroidSize;

typedef struct {
    Vector2 pos;
    int sides;
    float radius, rotation;
    Vector2 vel;
    float mass;

    int active;
    int hits;
    int maxHits;
    AsteroidSize size;
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

Asteroid ASTEROIDS[MAX_ASTEROIDS];
Bullet BULLETS[NUM_BULLETS];
int bulletActive[NUM_BULLETS];

Vector2 getRandV() {
    /**
     * Returns a random unit vector (random direction)
     */

    float angle = (float)GetRandomValue(0, 369) * DEG2RAD;
    return (Vector2){cosf(angle), sinf(angle)};
}

AsteroidSize getAsteroidSize(float r) {
    /**
     * Returns asteroid size enum based on radius size
     */
    if (r >= R_BIG)
        return AST_BIG;
    if (r >= R_MED)
        return AST_MED;
    return AST_SMALL;
}

int maxHitsFromSize(AsteroidSize s) {
    /**
     * Returns the number of hits required to break the asteroid depending on the size
     */
    switch (s) {
    case AST_SMALL:
        return 1;
    case AST_MED:
        return 2;
    case AST_BIG:
        return 3;
    }
    return 1; // should never happen
}

int findFreeAsteroidIndex() {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!ASTEROIDS[i].active) {
            return i;
        }
    }
    return -1; // All full
}

void createAsteroid(int i, Vector2 pos, float r, Vector2 velDir) {
    int sides = GetRandomValue(3, 8);
    float rotation = (float)GetRandomValue(1, 5);

    AsteroidSize s = getAsteroidSize(r);

    ASTEROIDS[i].pos = pos;
    ASTEROIDS[i].sides = sides;
    ASTEROIDS[i].radius = r;
    ASTEROIDS[i].rotation = rotation;
    ASTEROIDS[i].vel = Vector2Normalize(velDir);
    ASTEROIDS[i].mass = r * r;

    ASTEROIDS[i].active = 1;
    ASTEROIDS[i].hits = 0;
    ASTEROIDS[i].size = s;
    ASTEROIDS[i].maxHits = maxHitsFromSize(s);
}

void initAsteroids() {
    /**
     * Initialize asteroids with random positions/sizes/sides and random directions
     */

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        ASTEROIDS[i].active = 0;
    }

    for (int i = 0; i < NUM_START_ASTEROIDS; i++) {
        float radius = (float)GetRandomValue(35, 65);
        Vector2 pos;
        pos.x = (float)GetRandomValue((int)radius, (int)WIDTH - radius);
        pos.y = (float)GetRandomValue((int)radius, (int)HEIGHT - radius);

        int idx = findFreeAsteroidIndex();
        if (idx == -1)
            break;

        createAsteroid(idx, pos, radius, getRandV());
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
     * 5) Apply 1D elastic collision equations along the normal; tangential components remain
     * unchanged. 6) Reconstruct final velocity vectors
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

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        for (int j = i + 1; j < MAX_ASTEROIDS; j++) {
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
     * Update ship state
     */

    MoveSpaceship(s);
    s->pos.x += s->vel.x * VEL;
    s->pos.y += s->vel.y * VEL;
}

Vector2 initialSpaceshipPosition() {
    Vector2 center = {(float)WIDTH / 2, (float)HEIGHT / 2};
    Vector2 avg = {0.0f, 0.0f};
    for (int i = 0; i < NUM_START_ASTEROIDS; i++) {
        avg.x += ASTEROIDS[i].pos.x;
        avg.y += ASTEROIDS[i].pos.y;
    }
    avg.x /= NUM_START_ASTEROIDS;
    avg.y /= NUM_START_ASTEROIDS;

    Vector2 dir = {center.x - avg.x, center.y - avg.y};
    Vector2 shipPos = {center.x + dir.x, center.y + dir.y};

    if (shipPos.x < 0)
        shipPos.x = 0;
    if (shipPos.y < 0)
        shipPos.y = 0;
    if (shipPos.x > WIDTH)
        shipPos.x = WIDTH;
    if (shipPos.y > HEIGHT)
        shipPos.y = HEIGHT;

    return shipPos;
}

void splitAsteroid(int parentIdx) {
    Asteroid *p = &ASTEROIDS[parentIdx];
    if (!p->active)
        return;

    if (p->radius <= R_SMALL) {
        p->active = 0;
        return;
    }

    int childCount = 0;
    float childRadius = 0.0f;

    switch (p->size) {
    case AST_BIG:
        childCount = 3;
        childRadius = p->radius * 0.55f;
        break;
    case AST_MED:
        childCount = 2;
        childRadius = p->radius * 0.60f;
        break;
    default:
        p->active = 0;
        return;
    }

    Vector2 initPos = p->pos;
    p->active = 0;

    for (int i = 0; i < childCount; i++) {
        int idx = findFreeAsteroidIndex();
        if (idx == -1)
            return;

        Vector2 jitter = Vector2Scale(getRandV(), childRadius * 0.35f);
        Vector2 cPos = Vector2Add(initPos, jitter);

        Vector2 velDir = getRandV();
        createAsteroid(idx, cPos, childRadius, velDir);
        ASTEROIDS[idx].vel = Vector2Scale(ASTEROIDS[idx].vel, 1.3f);
    }
}

void UpdateAsteroids() {
    /**
     * Update asteroid positions, screen wrap-around, and visual rotation
     */

    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        Asteroid *a = &ASTEROIDS[i];
        if (!a->active)
            continue;

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

void handleBulletAsteroidCollisions(int *score) {
    /**
     * Checks for collisions between bullets and asteroids.
     */
    for (int bulletIdx = 0; bulletIdx < NUM_BULLETS; bulletIdx++) {
        if (!bulletActive[bulletIdx])
            continue;
        Bullet *b = &BULLETS[bulletIdx];

        for (int astIdx = 0; astIdx < MAX_ASTEROIDS; astIdx++) {
            Asteroid *a = &ASTEROIDS[astIdx];
            if (!a->active)
                continue;

            float dx = b->pos.x - a->pos.x;
            float dy = b->pos.y - a->pos.y;
            float r = b->radius + a->radius;

            if (dx * dx + dy * dy <= r * r) {
                bulletActive[bulletIdx] = 0;
                a->hits++;
                if (a->hits >= a->maxHits) {
                    splitAsteroid(astIdx);
                    (*score) += 10;
                }
                break;
            }
        }
    }
}

void createBullet(Vector2 pos, Vector2 velDir) {
    /**
     * Creates a bullet at the given position and direction if SPACE is pressed.
     */

    for (int i = 0; i < NUM_BULLETS; i++) {
        if (!bulletActive[i]) {
            BULLETS[i] = (Bullet){pos, 3, velDir};
            bulletActive[i] = 1;
            return;
        }
    }

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

void updateBullets() {
    /**
     * Deactivates bullets that go off-screen or hit an asteroid.
     */

    for (int i = 0; i < NUM_BULLETS; i++) {
        if (!bulletActive[i])
            continue;
        Bullet *b = &BULLETS[i];

        if (b->pos.x + b->radius < 0 || b->pos.x - b->radius > WIDTH || b->pos.y + b->radius < 0 ||
            b->pos.y - b->radius > HEIGHT) {
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

void tempDisableShooting(float seconds, int *shootingEnabled, double *startTime) {
    /**
     * Cooldown for shooting bullets
     */

    double currentTime = GetTime();

    if (!(*shootingEnabled)) {
        if ((currentTime - *startTime) >= seconds)
            *shootingEnabled = 1;
    }
}

void Shoot(Spaceship *s, int *score, int *shootingEnabled, double *startTime) {
    /**
     * Handles bullet creation, update, movement, and rendering.
     */

    if (*shootingEnabled && IsKeyPressed(KEY_SPACE)) {
        Vector2 position = s->pos;
        createBullet(position, (Vector2){1, 0});
        *shootingEnabled = 0;
        *startTime = GetTime();
    }
    moveBullet();
    updateBullets();
    handleBulletAsteroidCollisions(score);
    drawBullets();
}

void checkGameOver(Spaceship *s, int *gameOver) {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        Asteroid *a = &ASTEROIDS[i];
        if (!a->active)
            continue;
        float dx = s->pos.x - a->pos.x;
        float dy = s->pos.y - a->pos.y;
        float r = s->radius + a->radius;
        if (dx * dx + dy * dy <= r * r) {
            *gameOver = 1;
            return;
        }
    }
}

int checkWin() {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (ASTEROIDS[i].active)
            return 0;
    }
    return 1;
}

void UpdateGame(Spaceship *s) {
    UpdateAsteroids();
    UpdateSpaceship(s);
    checkCollisions();
}

void DrawSpaceShip(Spaceship *s) { DrawPoly(s->pos, 3, s->radius, 120, BLUE); }

void DrawAsteroids() {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        Asteroid *a = &ASTEROIDS[i];
        if (a->active) {
            DrawPoly(a->pos, a->sides, a->radius, a->rotation, RAYWHITE);
        }
    }
}

void DrawScore(int *score) { DrawText(TextFormat("Score: %d", *score), 0, 0, 40, GREEN); }

Spaceship initSpaceship() { return (Spaceship){initialSpaceshipPosition(), 10, (Vector2){0, 0}}; }

void restartGame(int *gameOver, int *score, Spaceship *spaceship) {
    initAsteroids();
    *spaceship = initSpaceship();
    *score = 0;
    *gameOver = 0;
}

int main(void) {
    InitWindow(WIDTH, HEIGHT, "Asteroid");
    SetTargetFPS(60);

    initAsteroids();
    Spaceship spaceship = initSpaceship();

    int gameOver = 0;
    int score = 0;
    int shootingEnabled = 1;
    double shootingStartTime = 0.0f;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        if (!gameOver) {
            DrawScore(&score);
            UpdateGame(&spaceship);
            DrawAsteroids();
            DrawSpaceShip(&spaceship);
            tempDisableShooting(0.3f, &shootingEnabled, &shootingStartTime);
            Shoot(&spaceship, &score, &shootingEnabled, &shootingStartTime);
            checkGameOver(&spaceship, &gameOver);

            if (checkWin()) {
                DrawText("You Win!", WIDTH / 2 - MeasureText("You Win!", 40) / 2, HEIGHT / 2 - 20,
                         40, GREEN);
                DrawText("Press [R] to Restart",
                         WIDTH / 2 - MeasureText("Press [R] to Restart", 40) / 2, HEIGHT / 2 + 60,
                         40, GREEN);

                if (IsKeyPressed(KEY_R)) {
                    restartGame(&gameOver, &score, &spaceship);
                }
            }

        } else {
            DrawText("Game Over", WIDTH / 2 - MeasureText("Game Over", 40) / 2, HEIGHT / 2 - 20, 40,
                     RED);
            DrawText("Press [R] to Restart",
                     WIDTH / 2 - MeasureText("Press [R] to Restart", 40) / 2, HEIGHT / 2 + 60, 40,
                     GREEN);

            if (IsKeyPressed(KEY_R))
                restartGame(&gameOver, &score, &spaceship);
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
