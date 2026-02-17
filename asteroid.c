#include "raylib.h"
#include "raymath.h"

#include <math.h>
#include <stdlib.h>

// CONSTANTS
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

#define MAX_STARS 100
#define MAX_PARTICLES 200

// TYPES
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

typedef struct {
    Vector2 pos;
    float brightness;
    float phase;
} Star;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float lifetime;
    float maxLifetime;
    Color color;
    float size;
} Particle;

// GAME STATE / GLOBAL STATE
Asteroid ASTEROIDS[MAX_ASTEROIDS];
Bullet BULLETS[NUM_BULLETS];
int bulletActive[NUM_BULLETS];
Star STARS[MAX_STARS];
Particle PARTICLES[MAX_PARTICLES];

// STARS
void initStars() {
    for (int i = 0; i < MAX_STARS; i++) {
        STARS[i].pos = (Vector2){(float)GetRandomValue(0, WIDTH), (float)GetRandomValue(0, HEIGHT)};
        STARS[i].brightness = (float)GetRandomValue(50, 150) / 255.0f;
        STARS[i].phase = (float)GetRandomValue(0, 360);
    }
}

void drawStars() {
    for (int i = 0; i < MAX_STARS; i++) {
        STARS[i].phase += 0.02f;
        float twinkle = 0.7f + 0.3f * sinf(STARS[i].phase);
        float alpha = STARS[i].brightness * twinkle;

        Color starColor = (Color){200, 200, 255, (alpha * 255)};
        DrawPixel(STARS[i].pos.x, STARS[i].pos.y, starColor);

        if (i % 7 == 0) {
            DrawPixel(STARS[i].pos.x + 1, STARS[i].pos.y, (Color){200, 200, 255, (alpha * 128)});
        }
    }
}

// PARTICLES
void createParticle(Vector2 pos, Vector2 vel, Color color, float lifetime, float size) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (PARTICLES[i].lifetime <= 0) {
            PARTICLES[i].pos = pos;
            PARTICLES[i].vel = vel;
            PARTICLES[i].color = color;
            PARTICLES[i].size = size;
            PARTICLES[i].lifetime = lifetime;
            PARTICLES[i].maxLifetime = lifetime;
            return;
        }
    }
}

void updateParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (PARTICLES[i].lifetime > 0) {
            PARTICLES[i].pos.x += PARTICLES[i].vel.x;
            PARTICLES[i].pos.y += PARTICLES[i].vel.y;
            PARTICLES[i].lifetime -= GetFrameTime();

            float lifeRatio = PARTICLES[i].lifetime / PARTICLES[i].maxLifetime;
            PARTICLES[i].color.a = 255 * lifeRatio;
        }
    }
}

void drawParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (PARTICLES[i].lifetime > 0) {
            float lifeRatio = PARTICLES[i].lifetime / PARTICLES[i].maxLifetime;
            float currentSize = PARTICLES[i].size * lifeRatio;

            DrawCircleV(PARTICLES[i].pos, currentSize * 1.5f,
                        Fade(PARTICLES[i].color, 0.3f * lifeRatio));
            DrawCircleV(PARTICLES[i].pos, currentSize, PARTICLES[i].color);
        }
    }
}

// HELPERS
Vector2 getRandV() {
    float angle = (float)GetRandomValue(0, 369) * DEG2RAD;
    return (Vector2){cosf(angle), sinf(angle)};
}

AsteroidSize getAsteroidSize(float r) {
    if (r >= R_BIG)
        return AST_BIG;
    if (r >= R_MED)
        return AST_MED;
    return AST_SMALL;
}

int maxHitsFromSize(AsteroidSize s) {
    switch (s) {
    case AST_SMALL:
        return 1;
    case AST_MED:
        return 2;
    case AST_BIG:
        return 3;
    }
    return 1;
}

int findFreeAsteroidIndex() {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        if (!ASTEROIDS[i].active) {
            return i;
        }
    }
    return -1;
}

// ASTEROIDS
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

    float overlap = rsum - dist;
    float invMa = (a->mass > 0) ? 1.0f / a->mass : 0.0f;
    float invMb = (b->mass > 0) ? 1.0f / b->mass : 0.0f;
    float invSum = invMa + invMb;
    if (invSum > 0.0f) {
        Vector2 correction = Vector2Scale(unitNormal, overlap / invSum);
        a->pos = Vector2Add(a->pos, Vector2Scale(correction, invMa));
        b->pos = Vector2Subtract(b->pos, Vector2Scale(correction, invMb));
    }

    float va_n = Vector2DotProduct(a->vel, unitNormal);
    float va_t = Vector2DotProduct(a->vel, unitTangent);
    float vb_n = Vector2DotProduct(b->vel, unitNormal);
    float vb_t = Vector2DotProduct(b->vel, unitTangent);

    float va_np = (va_n * (a->mass - b->mass) + 2.0f * b->mass * vb_n) / (a->mass + b->mass);
    float vb_np = (vb_n * (b->mass - a->mass) + 2.0f * a->mass * va_n) / (a->mass + b->mass);

    Vector2 va_np_vector = Vector2Scale(unitNormal, va_np);
    Vector2 va_tp_vector = Vector2Scale(unitTangent, va_t);
    Vector2 vb_np_vector = Vector2Scale(unitNormal, vb_np);
    Vector2 vb_tp_vector = Vector2Scale(unitTangent, vb_t);

    Vector2 velFinal_A = Vector2Add(va_np_vector, va_tp_vector);
    Vector2 velFinal_B = Vector2Add(vb_np_vector, vb_tp_vector);

    a->vel = velFinal_A;
    b->vel = velFinal_B;

    Vector2 collisionPoint = Vector2Add(a->pos, Vector2Scale(unitNormal, -a->radius));
    for (int i = 0; i < 3; i++) {
        Vector2 pVel = Vector2Scale(getRandV(), 1.5f);
        createParticle(collisionPoint, pVel, (Color){255, 200, 100, 255}, 0.5f, 2.0f);
    }
}

void checkCollisions() {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        for (int j = i + 1; j < MAX_ASTEROIDS; j++) {
            Asteroid *a = &ASTEROIDS[i];
            Asteroid *b = &ASTEROIDS[j];

            resolveCollisions(a, b);
        }
    }
}

void splitAsteroid(int parentIdx) {
    Asteroid *p = &ASTEROIDS[parentIdx];
    if (!p->active)
        return;

    int numParticles = (int)(p->radius / 5);
    for (int i = 0; i < numParticles; i++) {
        Vector2 pVel = Vector2Scale(getRandV(), 3.0f);
        Color particleColor =
            GetRandomValue(0, 1) ? (Color){255, 150, 50, 255} : (Color){255, 100, 30, 255};
        createParticle(p->pos, pVel, particleColor, 1.0f, 4.0f);
    }

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

void DrawAsteroids() {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        Asteroid *a = &ASTEROIDS[i];
        if (a->active) {
            Color baseColor = RAYWHITE;
            if (a->hits >= a->maxHits - 1) {
                baseColor = (Color){255, 150, 150, 255};
            } else if (a->hits > 0) {
                baseColor = (Color){255, 200, 200, 255};
            }
            DrawPoly(a->pos, a->sides, a->radius, a->rotation, baseColor);
        }
    }
}

// SPACESHIP
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

Spaceship initSpaceship() { return (Spaceship){initialSpaceshipPosition(), 10, (Vector2){0, 0}}; }

void MoveSpaceship(Spaceship *s) {
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
    MoveSpaceship(s);
    s->pos.x += s->vel.x * VEL;
    s->pos.y += s->vel.y * VEL;
}

void DrawSpaceShip(Spaceship *s) {
    DrawPoly(s->pos, 3, s->radius * 1.1f, 120, (Color){100, 180, 255, 255});
    DrawPoly(s->pos, 3, s->radius, 120, (Color){150, 200, 255, 255});
    DrawPoly(s->pos, 3, s->radius * 0.6f, 120, (Color){200, 230, 255, 255});
}

// BULLETS
void createBullet(Vector2 pos, Vector2 velDir) {
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
    for (int i = 0; i < NUM_BULLETS; i++) {
        if (!bulletActive[i])
            continue;
        Bullet *b = &BULLETS[i];
        b->pos.x += b->vel.x * BULLET_SPEED;
        b->pos.y += b->vel.y * BULLET_SPEED;
    }
}

void handleBulletAsteroidCollisions(int *score) {
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

void tempDisableShooting(float seconds, int *shootingEnabled, double *startTime) {
    double currentTime = GetTime();

    if (!(*shootingEnabled)) {
        if ((currentTime - *startTime) >= seconds)
            *shootingEnabled = 1;
    }
}

void Shoot(Spaceship *s, int *score, int *shootingEnabled, double *startTime) {
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

// GAME-OVER / WIN
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

void DrawScore(int *score) {
    const char *text = TextFormat("SCORE: %d", *score);

    DrawText(text, 12, 12, 30, Fade(GREEN, 0.5f));
    DrawText(text, 10, 10, 30, GREEN);
}

void DrawGameOverScreen() {
    char *gameOverText = "GAME OVER";
    char *restartText = "Press [R] to Restart";
    int goWidth = MeasureText(gameOverText, 50);
    int restartWidth = MeasureText(restartText, 30);

    float pulse = 0.7f + 0.3f * sinf(GetTime() * 3.0f);
    DrawText(gameOverText, WIDTH / 2 - goWidth / 2 + 2, HEIGHT / 2 - 22, 50,
             Fade(RED, 0.5f * pulse));
    DrawText(gameOverText, WIDTH / 2 - goWidth / 2, HEIGHT / 2 - 25, 50, RED);

    DrawText(restartText, WIDTH / 2 - restartWidth / 2 + 2, HEIGHT / 2 + 42, 30,
             Fade(RAYWHITE, 0.5f));
    DrawText(restartText, WIDTH / 2 - restartWidth / 2, HEIGHT / 2 + 40, 30, RAYWHITE);
}

void DrawWinScreen() {
    char *winText = "VICTORY!";
    char *restartText = "Press [R] to Restart";
    int winWidth = MeasureText(winText, 50);
    int restartWidth = MeasureText(restartText, 30);

    DrawText(winText, WIDTH / 2 - winWidth / 2 + 2, HEIGHT / 2 - 22, 50, Fade(GREEN, 0.5f));
    DrawText(winText, WIDTH / 2 - winWidth / 2, HEIGHT / 2 - 25, 50, GREEN);

    DrawText(restartText, WIDTH / 2 - restartWidth / 2 + 2, HEIGHT / 2 + 42, 30,
             Fade(RAYWHITE, 0.5f));
    DrawText(restartText, WIDTH / 2 - restartWidth / 2, HEIGHT / 2 + 40, 30, RAYWHITE);
}

void restartGame(int *gameOver, int *score, Spaceship *spaceship) {
    initAsteroids();
    *spaceship = initSpaceship();
    *score = 0;
    *gameOver = 0;
}

// MAIN ENTRY POINT
int main(void) {
    InitWindow(WIDTH, HEIGHT, "Asteroid");
    SetTargetFPS(60);

    initAsteroids();
    initStars();
    Spaceship spaceship = initSpaceship();

    int gameOver = 0;
    int score = 0;
    int shootingEnabled = 1;
    double shootingStartTime = 0.0f;

    Color bgColor = (Color){5, 5, 15, 255};

    for (int i = 0; i < MAX_PARTICLES; i++)
        PARTICLES[i].lifetime = 0;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(bgColor);
        drawStars();
        if (!gameOver) {
            DrawScore(&score);
            UpdateAsteroids();
            UpdateSpaceship(&spaceship);
            checkCollisions();
            updateParticles();
            drawParticles();
            DrawAsteroids();
            DrawSpaceShip(&spaceship);
            tempDisableShooting(0.3f, &shootingEnabled, &shootingStartTime);
            Shoot(&spaceship, &score, &shootingEnabled, &shootingStartTime);
            checkGameOver(&spaceship, &gameOver);

            if (checkWin()) {
                DrawWinScreen();
                if (IsKeyPressed(KEY_R)) {
                    restartGame(&gameOver, &score, &spaceship);
                }
            }
        } else {
            DrawGameOverScreen();
            if (IsKeyPressed(KEY_R))
                restartGame(&gameOver, &score, &spaceship);
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
