// asteroid-web.c - WebAssembly/emscripten build
// Refactored from the original asteroid.c to work with emscripten.

#include "raylib.h"
#include "raymath.h"

// emscripten header (only included when compiling for the web)
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

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

Spaceship spaceship;
int gameOver = 0;
int score = 0;
int shootingEnabled = 1;
double shootingStartTime = 0.0;

Color bgColor = {5, 5, 15, 255};

// STARS
void initStars(void) {
    for (int i = 0; i < MAX_STARS; i++) {
        STARS[i].pos = (Vector2){(float)GetRandomValue(0, WIDTH), (float)GetRandomValue(0, HEIGHT)};
        STARS[i].brightness = (float)GetRandomValue(50, 150) / 255.0f;
        STARS[i].phase = (float)GetRandomValue(0, 360);
    }
}

void drawStars(void) {
    for (int i = 0; i < MAX_STARS; i++) {
        STARS[i].phase += 0.02f;
        float twinkle = 0.7f + 0.3f * sinf(STARS[i].phase);
        float alpha = STARS[i].brightness * twinkle;

        Color starColor = {200, 200, 255, (unsigned char)(alpha * 255)};
        DrawPixel((int)STARS[i].pos.x, (int)STARS[i].pos.y, starColor);

        if (i % 7 == 0)
            DrawPixel((int)STARS[i].pos.x + 1, (int)STARS[i].pos.y,
                      (Color){200, 200, 255, (unsigned char)(alpha * 128)});
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

void updateParticles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (PARTICLES[i].lifetime > 0) {
            PARTICLES[i].pos.x += PARTICLES[i].vel.x;
            PARTICLES[i].pos.y += PARTICLES[i].vel.y;
            PARTICLES[i].lifetime -= GetFrameTime();

            float lifeRatio = PARTICLES[i].lifetime / PARTICLES[i].maxLifetime;
            PARTICLES[i].color.a = (unsigned char)(255 * lifeRatio);
        }
    }
}

void drawParticles(void) {
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
Vector2 getRandV(void) {
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

int findFreeAsteroidIndex(void) {
    for (int i = 0; i < MAX_ASTEROIDS; i++)
        if (!ASTEROIDS[i].active)
            return i;
    return -1;
}

// ASTEROIDS
void createAsteroid(int i, Vector2 pos, float r, Vector2 velDir) {
    AsteroidSize s = getAsteroidSize(r);
    ASTEROIDS[i].pos = pos;
    ASTEROIDS[i].sides = GetRandomValue(3, 8);
    ASTEROIDS[i].radius = r;
    ASTEROIDS[i].rotation = (float)GetRandomValue(1, 5);
    ASTEROIDS[i].vel = Vector2Normalize(velDir);
    ASTEROIDS[i].mass = r * r;
    ASTEROIDS[i].active = 1;
    ASTEROIDS[i].hits = 0;
    ASTEROIDS[i].size = s;
    ASTEROIDS[i].maxHits = maxHitsFromSize(s);
}

void initAsteroids(void) {
    for (int i = 0; i < MAX_ASTEROIDS; i++)
        ASTEROIDS[i].active = 0;

    for (int i = 0; i < NUM_START_ASTEROIDS; i++) {
        float radius = (float)GetRandomValue(35, 65);
        Vector2 pos = {(float)GetRandomValue((int)radius, WIDTH - (int)radius),
                       (float)GetRandomValue((int)radius, HEIGHT - (int)radius)};
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
    Vector2 unitTangent = {-unitNormal.y, unitNormal.x};

    Vector2 rv = Vector2Subtract(a->vel, b->vel);
    if (Vector2DotProduct(rv, unitNormal) > 0.0f)
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

    a->vel = Vector2Add(Vector2Scale(unitNormal, va_np), Vector2Scale(unitTangent, va_t));
    b->vel = Vector2Add(Vector2Scale(unitNormal, vb_np), Vector2Scale(unitTangent, vb_t));

    Vector2 cp = Vector2Add(a->pos, Vector2Scale(unitNormal, -a->radius));
    for (int i = 0; i < 3; i++)
        createParticle(cp, Vector2Scale(getRandV(), 1.5f), (Color){255, 200, 100, 255}, 0.5f, 2.0f);
}

void checkCollisions(void) {
    for (int i = 0; i < MAX_ASTEROIDS; i++)
        for (int j = i + 1; j < MAX_ASTEROIDS; j++)
            resolveCollisions(&ASTEROIDS[i], &ASTEROIDS[j]);
}

void splitAsteroid(int parentIdx) {
    Asteroid *p = &ASTEROIDS[parentIdx];
    if (!p->active)
        return;

    int numParticles = (int)(p->radius / 5);
    for (int i = 0; i < numParticles; i++) {
        Color c = GetRandomValue(0, 1) ? (Color){255, 150, 50, 255} : (Color){255, 100, 30, 255};
        createParticle(p->pos, Vector2Scale(getRandV(), 3.0f), c, 1.0f, 4.0f);
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
        Vector2 cPos = Vector2Add(initPos, Vector2Scale(getRandV(), childRadius * 0.35f));
        createAsteroid(idx, cPos, childRadius, getRandV());
        ASTEROIDS[idx].vel = Vector2Scale(ASTEROIDS[idx].vel, 1.3f);
    }
}

void UpdateAsteroids(void) {
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

// SPACESHIP
Vector2 initialSpaceshipPosition(void) {
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

Spaceship initSpaceship(void) { return (Spaceship){initialSpaceshipPosition(), 10, {0, 0}}; }

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

void drawBullets(void) {
    for (int i = 0; i < NUM_BULLETS; i++) {
        if (!bulletActive[i])
            continue;
        DrawCircle((int)BULLETS[i].pos.x, (int)BULLETS[i].pos.y, BULLETS[i].radius, RAYWHITE);
    }
}

void updateBullets(void) {
    for (int i = 0; i < NUM_BULLETS; i++) {
        if (!bulletActive[i])
            continue;
        Bullet *b = &BULLETS[i];
        if (b->pos.x + b->radius < 0 || b->pos.x - b->radius > WIDTH || b->pos.y + b->radius < 0 ||
            b->pos.y - b->radius > HEIGHT)
            bulletActive[i] = 0;
    }
}

void moveBullet(void) {
    for (int i = 0; i < NUM_BULLETS; i++) {
        if (!bulletActive[i])
            continue;
        BULLETS[i].pos.x += BULLETS[i].vel.x * BULLET_SPEED;
        BULLETS[i].pos.y += BULLETS[i].vel.y * BULLET_SPEED;
    }
}

void handleBulletAsteroidCollisions(int *pScore) {
    for (int bi = 0; bi < NUM_BULLETS; bi++) {
        if (!bulletActive[bi])
            continue;
        Bullet *b = &BULLETS[bi];
        for (int ai = 0; ai < MAX_ASTEROIDS; ai++) {
            Asteroid *a = &ASTEROIDS[ai];
            if (!a->active)
                continue;
            float dx = b->pos.x - a->pos.x;
            float dy = b->pos.y - a->pos.y;
            float r = b->radius + a->radius;
            if (dx * dx + dy * dy <= r * r) {
                bulletActive[bi] = 0;
                a->hits++;
                if (a->hits >= a->maxHits) {
                    splitAsteroid(ai);
                    (*pScore) += 10;
                }
                break;
            }
        }
    }
}

void tempDisableShooting(float seconds, int *enabled, double *startTime) {
    if (!(*enabled) && (GetTime() - *startTime) >= seconds)
        *enabled = 1;
}

void Shoot(Spaceship *s, int *pScore, int *enabled, double *startTime) {
    if (*enabled && IsKeyPressed(KEY_SPACE)) {
        createBullet(s->pos, (Vector2){1, 0});
        *enabled = 0;
        *startTime = GetTime();
    }
    moveBullet();
    updateBullets();
    handleBulletAsteroidCollisions(pScore);
    drawBullets();
}

// GAME-OVER / WIN
void checkGameOver(Spaceship *s, int *pGameOver) {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        Asteroid *a = &ASTEROIDS[i];
        if (!a->active)
            continue;
        float dx = s->pos.x - a->pos.x;
        float dy = s->pos.y - a->pos.y;
        float r = s->radius + a->radius;
        if (dx * dx + dy * dy <= r * r) {
            *pGameOver = 1;
            return;
        }
    }
}

int checkWin(void) {
    for (int i = 0; i < MAX_ASTEROIDS; i++)
        if (ASTEROIDS[i].active)
            return 0;
    return 1;
}

void DrawScore(int *pScore) {
    const char *text = TextFormat("SCORE: %d", *pScore);
    DrawText(text, 12, 12, 30, Fade(GREEN, 0.5f));
    DrawText(text, 10, 10, 30, GREEN);
}

void DrawGameOverScreen(void) {
    const char *goText = "GAME OVER";
    const char *restartText = "Press [R] to Restart";
    int goWidth = MeasureText(goText, 50);
    int restartWidth = MeasureText(restartText, 30);
    float pulse = 0.7f + 0.3f * sinf((float)GetTime() * 3.0f);
    DrawText(goText, WIDTH / 2 - goWidth / 2 + 2, HEIGHT / 2 - 22, 50, Fade(RED, 0.5f * pulse));
    DrawText(goText, WIDTH / 2 - goWidth / 2, HEIGHT / 2 - 25, 50, RED);
    DrawText(restartText, WIDTH / 2 - restartWidth / 2 + 2, HEIGHT / 2 + 42, 30,
             Fade(RAYWHITE, 0.5f));
    DrawText(restartText, WIDTH / 2 - restartWidth / 2, HEIGHT / 2 + 40, 30, RAYWHITE);
}

void DrawWinScreen(void) {
    const char *winText = "VICTORY!";
    const char *restartText = "Press [R] to Restart";
    int winWidth = MeasureText(winText, 50);
    int restartWidth = MeasureText(restartText, 30);
    DrawText(winText, WIDTH / 2 - winWidth / 2 + 2, HEIGHT / 2 - 22, 50, Fade(GREEN, 0.5f));
    DrawText(winText, WIDTH / 2 - winWidth / 2, HEIGHT / 2 - 25, 50, GREEN);
    DrawText(restartText, WIDTH / 2 - restartWidth / 2 + 2, HEIGHT / 2 + 42, 30,
             Fade(RAYWHITE, 0.5f));
    DrawText(restartText, WIDTH / 2 - restartWidth / 2, HEIGHT / 2 + 40, 30, RAYWHITE);
}

void restartGame(void) {
    initAsteroids();
    spaceship = initSpaceship();
    score = 0;
    gameOver = 0;
    shootingEnabled = 1;
}

// UpdateDrawFrame (for web animation)
void DrawAsteroids(void) {
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        Asteroid *a = &ASTEROIDS[i];
        if (!a->active)
            continue;
        Color baseColor = RAYWHITE;
        if (a->hits >= a->maxHits - 1)
            baseColor = (Color){255, 150, 150, 255};
        else if (a->hits > 0)
            baseColor = (Color){255, 200, 200, 255};
        DrawPoly(a->pos, a->sides, a->radius, a->rotation, baseColor);
    }
}

void UpdateDrawFrame(void) {
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
            if (IsKeyPressed(KEY_R))
                restartGame();
        }
    } else {
        DrawGameOverScreen();
        if (IsKeyPressed(KEY_R))
            restartGame();
    }
    EndDrawing();
}

// MAIN ENTRY POINT
int main(void) {
    InitWindow(WIDTH, HEIGHT, "Asteroid");
    SetTargetFPS(60);

    for (int i = 0; i < MAX_PARTICLES; i++)
        PARTICLES[i].lifetime = 0;
    initAsteroids();
    initStars();
    spaceship = initSpaceship();

#if defined(PLATFORM_WEB)
    // WEB
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    // DESKTOP
    while (!WindowShouldClose())
        UpdateDrawFrame();
#endif

    CloseWindow();
    return 0;
}