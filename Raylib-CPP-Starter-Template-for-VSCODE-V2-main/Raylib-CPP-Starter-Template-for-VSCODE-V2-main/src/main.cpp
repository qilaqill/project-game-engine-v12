#include "raylib.h"
#include <vector>
#include <string>
#include <cmath>

// --- SETTINGS ---
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float GRAVITY = 0.8f;
const float JUMP_FORCE = -14.0f;
const float PLAYER_SPEED = 5.0f;
const float BULLET_SPEED = 10.0f;

enum GameState { MENU, LEVEL_1, LEVEL_2, GAME_OVER, WIN };

// --- AUDIO ASSETS (Global for access) ---
Sound fxJump;
Sound fxShoot;
Music bgmMusic;

// --- 1. REQUIREMENT: CLASS-BASED PROGRAMMING ---
// Kita bungkus data & function ke dalam Class

class Player {
public:
    Rectangle rect;
    Vector2 velocity;
    bool isGrounded;
    bool facingRight;

    void Reset(float x, float y) {
        rect = { x, y, 40, 40 };
        velocity = { 0, 0 };
        isGrounded = false;
        facingRight = true;
    }

    void Update(const std::vector<struct Platform>& platforms); 
    void Draw() {
        DrawRectangle(rect.x + 8, rect.y + 15, 24, 25, BLUE);
        DrawCircle(rect.x + 20, rect.y + 10, 10, BLUE);
        DrawText("Player", rect.x, rect.y - 20, 10, BLUE);
    }
};

class Enemy {
public:
    enum Type { PATROL, CHASE };
    Type type;
    Rectangle rect;
    Vector2 startPos;
    float patrolDist;
    float speed;
    int direction;
    bool active;

    void Update(Vector2 playerPos);
    void Draw() {
        if (!active) return;
        Color enemyColor = (type == PATROL) ? RED : ORANGE;
        DrawRectangle(rect.x + 8, rect.y + 15, 24, 25, enemyColor);
        DrawCircle(rect.x + 20, rect.y + 10, 10, enemyColor);
        
        // MATA ENEMY (Kekalkan logic asal)
        if (type == PATROL) {
            DrawCircle(rect.x + 16, rect.y + 10, 2, BLACK);
            DrawCircle(rect.x + 24, rect.y + 10, 2, BLACK);
        } else {
            DrawCircle(rect.x + 16, rect.y + 10, 3, RED);
            DrawCircle(rect.x + 24, rect.y + 10, 3, RED);
        }
        DrawText(type == PATROL ? "Patrol" : "Chase", rect.x, rect.y - 10, 10, enemyColor);
    }
};

struct Platform {
    Rectangle rect;
    Color color;
};

struct Bullet {
    Vector2 position;
    Vector2 speed;
    bool active;
    float radius;
};

struct Star {
    Rectangle rect;
    bool active;
};

// --- GLOBAL VARIABLES ---
GameState currentState = MENU;
Player player;
std::vector<Platform> platforms;
std::vector<Enemy> enemies;
std::vector<Bullet> bullets;
std::vector<Star> stars;
int starsCollected = 0;
int totalStarsInLevel = 0;

// --- PROTOTYPES ---
void InitLevel(int level);
void DrawPlatformFancy(Rectangle rect);



// --- CLASS METHODS IMPLEMENTATION ---
void Player::Update(const std::vector<Platform>& platforms) {
    if (IsKeyDown(KEY_LEFT)) { velocity.x = -PLAYER_SPEED; facingRight = false; }
    else if (IsKeyDown(KEY_RIGHT)) { velocity.x = PLAYER_SPEED; facingRight = true; }
    else velocity.x = 0;

    if (IsKeyPressed(KEY_SPACE) && isGrounded) {
        velocity.y = JUMP_FORCE;
        isGrounded = false;
        PlaySound(fxJump);
    }

    if (IsKeyPressed(KEY_Z)) {
        Bullet b = { { rect.x + 20, rect.y + 20 }, { facingRight ? BULLET_SPEED : -BULLET_SPEED, 0 }, true, 5 };
        bullets.push_back(b);
        PlaySound(fxShoot);
    }

    velocity.y += GRAVITY;
    rect.x += velocity.x;
    if (rect.x < 0) rect.x = 0;

    isGrounded = false;
    rect.y += velocity.y;

    for (const auto& plat : platforms) {
        if (CheckCollisionRecs(rect, plat.rect)) {
            if (velocity.y > 0 && rect.y + rect.height - velocity.y <= plat.rect.y) {
                rect.y = plat.rect.y - rect.height;
                velocity.y = 0;
                isGrounded = true;
            }
        }
    }
    if (rect.y > SCREEN_HEIGHT) currentState = GAME_OVER;
}

void Enemy::Update(Vector2 playerPos) {
    if (!active) return;
    if (type == PATROL) {
        rect.x += speed * direction;
        if (rect.x > startPos.x + patrolDist || rect.x < startPos.x) direction *= -1;
    } else if (type == CHASE) {
        float distance = sqrt(pow(playerPos.x - rect.x, 2) + pow(playerPos.y - rect.y, 2));
        if (distance < 300) {
            if (playerPos.x > rect.x) rect.x += speed;
            else rect.x -= speed;
        }
    }
}

// --- MAIN GAME FUNCTIONS ---
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Final Project: 2D Platformer Engine");
    InitAudioDevice();

    fxJump = LoadSound("jump.mp3");
    fxShoot = LoadSound("shoot.mp3");
    bgmMusic = LoadMusicStream("bgm.mp3");
    PlayMusicStream(bgmMusic);
    SetMusicVolume(bgmMusic, 0.5f);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateMusicStream(bgmMusic);
        
        if (currentState == MENU && IsKeyPressed(KEY_ENTER)) { InitLevel(1); currentState = LEVEL_1; }
        else if (currentState == GAME_OVER || currentState == WIN) { if (IsKeyPressed(KEY_R)) currentState = MENU; }
        else if (currentState == LEVEL_1 || currentState == LEVEL_2) {
            // Update All
            player.Update(platforms);
            for (auto& e : enemies) {
                e.Update({player.rect.x, player.rect.y});
                if (CheckCollisionRecs(player.rect, e.rect) && e.active) currentState = GAME_OVER;
            }
            
            // Bullets logic
            for (auto& b : bullets) {
                if (!b.active) continue;
                b.position.x += b.speed.x;
                if (b.position.x < 0 || b.position.x > SCREEN_WIDTH) b.active = false;
                for (auto& e : enemies) {
                    if (e.active && CheckCollisionCircleRec(b.position, b.radius, e.rect)) {
                        e.active = false; b.active = false;
                    }
                }
            }

            // Stars logic
            for (auto& s : stars) {
                if (s.active && CheckCollisionRecs(player.rect, s.rect)) {
                    s.active = false; starsCollected++;
                }
            }

            // Level Win
            if (player.rect.x > SCREEN_WIDTH - 50) {
                if (currentState == LEVEL_1) { currentState = LEVEL_2; InitLevel(2); }
                else currentState = WIN;
            }
        }

        // DRAWING
        BeginDrawing();
        ClearBackground(Color{ 135, 206, 235, 255 }); 

        // BACKGROUND POKOK (Kekalkan)
        for (int i = 0; i < SCREEN_WIDTH; i += 120) DrawTriangle({ (float)i + 60, 120 }, { (float)i, 360 }, { (float)i + 120, 360 }, Color{ 120, 180, 120, 255 });
        for (int i = 0; i < SCREEN_WIDTH; i += 90) DrawTriangle({ (float)i + 45, 180 }, { (float)i, 420 }, { (float)i + 90, 420 }, DARKGREEN);
        DrawRectangle(0, 420, SCREEN_WIDTH, 200, Fade(DARKGREEN, 0.15f));

        if (currentState == MENU) {
            DrawText("GAME ENGINE II FINAL PROJECT", 180, 100, 30, DARKBLUE);
            DrawText("Group Members:", 200, 200, 20, GRAY);
            DrawText("1. SYED MUHAMMAD BIN SYED NAJIB (B032310056)", 200, 230, 20, BLACK);
            DrawText("2. MUHAMMAD AQIL BIN MOKHTAR (B032310493)", 200, 260, 20, BLACK);
            DrawText("3. MUHAMMAD IRFAN BIN MOHD NADZARUDIN (B032310316)", 200, 290, 20, BLACK);
            DrawText("4. MUHAMMAD NAZMI BIN ZULKEFLI (B032310837)", 200, 320, 20, BLACK);
            DrawText("PRESS [ENTER] TO START", 250, 450, 20, RED);
        } else if (currentState == GAME_OVER) {
            DrawText("GAME OVER", 300, 250, 40, RED);
            DrawText("PRESS [R] TO RESTART", 280, 320, 20, DARKGRAY);
        } else if (currentState == WIN) {
            DrawText("YOU WIN!", 320, 250, 40, GREEN);
        } else {
            for (const auto& plat : platforms) DrawPlatformFancy(plat.rect);
            for (const auto& s : stars) {
                if (s.active) {
                    DrawPoly({ s.rect.x + 10, s.rect.y + 10 }, 5, 12, 0, GOLD);
                    DrawCircleLines(s.rect.x + 10, s.rect.y + 10, 13, Fade(YELLOW, 0.5f));
                }
            }
            player.Draw();
            for (auto& e : enemies) e.Draw();
            for (const auto& b : bullets) if (b.active) DrawCircleV(b.position, b.radius, BLACK);
           // DrawText(TextFormat("Level: %d", (currentState == LEVEL_1 ? 1 : 2)), 10, 40, 20, BLACK);
            //DrawText(TextFormat("Stars: %d", starsCollected), 10, 70, 20, GOLD);
        }

        // --- UI & TUTORIAL MECHANIC ---
        // Letak ni paling bawah supaya dia sentiasa di depan (layer atas sekali)
        DrawRectangle(5, 5, 520, 95, Fade(BLACK, 0.3f)); // Background kotak hitam sikit supaya teks nampak
        DrawText("CONTROLS: ARROWS to Move, SPACE to Jump, Z to Shoot", 15, 15, 18, WHITE);
        DrawText(TextFormat("LEVEL: %d", (currentState == LEVEL_1 ? 1 : 2)), 15, 40, 18, YELLOW);
        DrawText(TextFormat("STARS: %d", starsCollected), 15, 65, 18, GOLD);

        EndDrawing();
    }

    UnloadSound(fxJump); UnloadSound(fxShoot); UnloadMusicStream(bgmMusic);
    CloseAudioDevice(); CloseWindow();
    return 0;
}

void InitLevel(int level) {
    player.Reset(50, 400);
    bullets.clear(); enemies.clear(); platforms.clear(); stars.clear();
    starsCollected = 0;
    if (level == 1) {
        platforms.push_back({ { 0, 500, 200, 20 }, DARKGRAY });
        platforms.push_back({ { 250, 400, 150, 20 }, GRAY });
        platforms.push_back({ { 500, 300, 150, 20 }, GRAY });
        platforms.push_back({ { 700, 250, 100, 20 }, GOLD });
        enemies.push_back({ Enemy::PATROL, { 250, 360, 40, 40 }, {250, 360}, 150, 2.0f, 1, true });
        stars.push_back({ { 315, 375, 20, 20 }, true });
    } else {
        platforms.push_back({ { 0, 500, 100, 20 }, DARKGRAY });
        platforms.push_back({ { 150, 450, 100, 20 }, GRAY });
        platforms.push_back({ { 300, 350, 100, 20 }, GRAY });
        platforms.push_back({ { 450, 250, 100, 20 }, GRAY });
        platforms.push_back({ { 650, 200, 150, 20 }, GOLD });
        enemies.push_back({ Enemy::CHASE, { 450, 210, 40, 40 }, {0,0}, 0, 1.5f, 1, true });
        enemies.push_back({ Enemy::PATROL, { 300, 310, 40, 40 }, {300, 310}, 100, 3.0f, 1, true });
        stars.push_back({ { 190, 425, 20, 20 }, true });
        stars.push_back({ { 490, 225, 20, 20 }, true });
    }
}

void DrawPlatformFancy(Rectangle rect) {
    DrawRectangleRec(rect, Color{ 139, 69, 19, 255 });
    DrawRectangle(rect.x, rect.y - 5, rect.width, 5, DARKGREEN);
    for (int i = 0; i < rect.width; i += 20) DrawLine(rect.x + i, rect.y + 5, rect.x + i + 10, rect.y + rect.height, Fade(BLACK, 0.2f));
}