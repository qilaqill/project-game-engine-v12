#include "raylib.h"
#include <vector>
#include <string>
#include <cmath>

// --- REQUIREMENT: 2.1 Gameplay Elements (Gravity, Physics) [cite: 10] ---
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float GRAVITY = 0.8f;
const float JUMP_FORCE = -14.0f;
const float PLAYER_SPEED = 5.0f;
const float BULLET_SPEED = 10.0f;

// --- REQUIREMENT: 2.2 Scene & Flow Requirements [cite: 17, 22] ---
enum GameState { MENU, LEVEL_1, LEVEL_2, GAME_OVER, WIN };

// --- Structs for Modularity [cite: 32] ---
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

struct Enemy {
    // REQUIREMENT: Two enemy types [cite: 12]
    enum Type { PATROL, CHASE };
    Type type;
    Rectangle rect;
    Vector2 startPos;
    float patrolDist; // How far they patrol
    float speed;
    int direction;    // 1 or -1
    bool active;
};

struct Player {
    Rectangle rect;
    Vector2 velocity;
    bool isGrounded;
    bool facingRight;
    int lives;
};

// --- Global Variables ---
GameState currentState = MENU;
Player player;
std::vector<Platform> platforms;
std::vector<Enemy> enemies;
std::vector<Bullet> bullets;

// Audio Assets 
Sound fxJump;
Sound fxShoot;
Music bgmMusic;

// --- Function Prototypes ---
void InitLevel(int level);
void UpdateGame();
void DrawGame();
void UpdatePlayer();
void UpdateEnemies();
void UpdateBullets();

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Final Project: 2D Platformer Engine");
    InitAudioDevice();

    // REQUIREMENT: Audio Requirements [cite: 23, 24, 25]
    // Note: Ensure these files exist in your folder, or Raylib will warn but continue.
    fxJump = LoadSound("jump.mp3");
    fxShoot = LoadSound("shoot.mp3");
    bgmMusic = LoadMusicStream("bgm.mp3");
    PlayMusicStream(bgmMusic);
    SetMusicVolume(bgmMusic, 0.5f);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateMusicStream(bgmMusic); // Keep BGM playing
        
        switch (currentState) {
            case MENU:
                if (IsKeyPressed(KEY_ENTER)) {
                    InitLevel(1);
                    currentState = LEVEL_1;
                }
                break;
            case LEVEL_1:
            case LEVEL_2:
                UpdateGame();
                break;
            case GAME_OVER:
            case WIN:
                if (IsKeyPressed(KEY_R)) currentState = MENU;
                break;
        }

        DrawGame();
    }

    // Cleanup
    UnloadSound(fxJump);
    UnloadSound(fxShoot);
    UnloadMusicStream(bgmMusic);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

void InitLevel(int level) {
    // Reset Player
    player.rect = { 50, 400, 40, 40 };
    player.velocity = { 0, 0 };
    player.isGrounded = false;
    player.facingRight = true;
    player.lives = 1;
    bullets.clear();
    enemies.clear();
    platforms.clear();

    // REQUIREMENT: Ground floor is hazardous (The void below is the hazard) [cite: 15]
    // We build platforms above the bottom of the screen.

    if (level == 1) {
        // Level 1: Simple Layout
        platforms.push_back({ { 0, 500, 200, 20 }, DARKGRAY });
        platforms.push_back({ { 250, 400, 150, 20 }, GRAY });
        platforms.push_back({ { 500, 300, 150, 20 }, GRAY });
        platforms.push_back({ { 700, 250, 100, 20 }, GOLD }); // Goal Platform

        // REQUIREMENT: Patrol Enemy [cite: 13]
        enemies.push_back({ Enemy::PATROL, { 250, 360, 40, 40 }, {250, 360}, 150, 2.0f, 1, true });
    } 
    else if (level == 2) {
        // Level 2: Harder Layout
        platforms.push_back({ { 0, 500, 100, 20 }, DARKGRAY });
        platforms.push_back({ { 150, 450, 100, 20 }, GRAY });
        platforms.push_back({ { 300, 350, 100, 20 }, GRAY });
        platforms.push_back({ { 450, 250, 100, 20 }, GRAY });
        platforms.push_back({ { 650, 200, 150, 20 }, GOLD }); // Goal Platform

        // REQUIREMENT: Chase Enemy [cite: 14]
        enemies.push_back({ Enemy::CHASE, { 450, 210, 40, 40 }, {0,0}, 0, 1.5f, 1, true });
        // Another Patrol
        enemies.push_back({ Enemy::PATROL, { 300, 310, 40, 40 }, {300, 310}, 100, 3.0f, 1, true });
    }
}

void UpdatePlayer() {
    // Movement
    if (IsKeyDown(KEY_LEFT)) {
        player.velocity.x = -PLAYER_SPEED;
        player.facingRight = false;
    } else if (IsKeyDown(KEY_RIGHT)) {
        player.velocity.x = PLAYER_SPEED;
        player.facingRight = true;
    } else {
        player.velocity.x = 0;
    }

    // Jumping [cite: 11]
    if (IsKeyPressed(KEY_SPACE) && player.isGrounded) {
        player.velocity.y = JUMP_FORCE;
        player.isGrounded = false;
        PlaySound(fxJump);
    }

    // REQUIREMENT: Shooting [cite: 11]
    if (IsKeyPressed(KEY_Z)) {
        Bullet b;
        b.position = { player.rect.x + 20, player.rect.y + 20 };
        b.speed = { player.facingRight ? BULLET_SPEED : -BULLET_SPEED, 0 };
        b.active = true;
        b.radius = 5;
        bullets.push_back(b);
        PlaySound(fxShoot);
    }

    // Apply Gravity
    player.velocity.y += GRAVITY;
    player.rect.x += player.velocity.x;
    
    // X-Collision (Screen Bounds)
    if (player.rect.x < 0) player.rect.x = 0;

    // Y-Collision (Platforms)
    player.isGrounded = false;
    player.rect.y += player.velocity.y;

    for (const auto& plat : platforms) {
        // Simple AABB collision detection
        if (CheckCollisionRecs(player.rect, plat.rect)) {
            // Landing on top
            if (player.velocity.y > 0 && player.rect.y + player.rect.height - player.velocity.y <= plat.rect.y) {
                player.rect.y = plat.rect.y - player.rect.height;
                player.velocity.y = 0;
                player.isGrounded = true;
            }
        }
    }

    // REQUIREMENT: Ground floor is hazardous (Fall Condition) [cite: 15]
    if (player.rect.y > SCREEN_HEIGHT) {
        currentState = GAME_OVER;
    }

    // REQUIREMENT: Win Condition / Level Switching [cite: 16]
    // Check if player reached the right side of the screen
    if (player.rect.x > SCREEN_WIDTH - 50) {
        if (currentState == LEVEL_1) {
            currentState = LEVEL_2;
            InitLevel(2);
        } else {
            currentState = WIN;
        }
    }
}

void UpdateEnemies() {
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;

        // REQUIREMENT: Patrol Logic [cite: 13]
        if (enemy.type == Enemy::PATROL) {
            enemy.rect.x += enemy.speed * enemy.direction;
            // Check boundaries relative to start position
            if (enemy.rect.x > enemy.startPos.x + enemy.patrolDist || 
                enemy.rect.x < enemy.startPos.x) {
                enemy.direction *= -1; // Flip direction
            }
        }
        // REQUIREMENT: Chase Logic [cite: 14]
        else if (enemy.type == Enemy::CHASE) {
            float distance = sqrt(pow(player.rect.x - enemy.rect.x, 2) + pow(player.rect.y - enemy.rect.y, 2));
            
            // Only chase if within 300 pixels (Proximity)
            if (distance < 300) {
                if (player.rect.x > enemy.rect.x) enemy.rect.x += enemy.speed;
                else enemy.rect.x -= enemy.speed;
            }
        }

        // Player vs Enemy Collision
        if (CheckCollisionRecs(player.rect, enemy.rect)) {
            currentState = GAME_OVER;
        }
    }
}

void UpdateBullets() {
    for (auto& b : bullets) {
        if (!b.active) continue;
        b.position.x += b.speed.x;
        b.position.y += b.speed.y;

        // Deactivate if off screen
        if (b.position.x < 0 || b.position.x > SCREEN_WIDTH) b.active = false;

        // Bullet vs Enemy Collision
        for (auto& e : enemies) {
            if (e.active && CheckCollisionCircleRec(b.position, b.radius, e.rect)) {
                e.active = false; // Kill enemy
                b.active = false; // Destroy bullet
            }
        }
    }
}

void UpdateGame() {
    UpdatePlayer();
    UpdateEnemies();
    UpdateBullets();
}

void DrawGame() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (currentState == MENU) {
        DrawText("GAME ENGINE II FINAL PROJECT", 180, 100, 30, DARKBLUE);
        // REQUIREMENT: Main Menu must list all group member names 
        DrawText("Group Members:", 200, 200, 20, GRAY);
        DrawText("1. [SYED MUHAMMAD BIN SYED NAJIB (B032310056)]", 200, 230, 20, BLACK);
        DrawText("2. [MUHAMMAD AQIL BIN MOKHTAR (B032310493)]", 200, 260, 20, BLACK);
        DrawText("3. [MUHAMMAD IRFAN BIN MOHD NADZARUDIN (B032310316)]", 200, 290, 20, BLACK);
        DrawText("4. [MUHAMMAD NAZMI BIN ZULKEFLI (B032310837)]", 200, 320, 20, BLACK);
        DrawText("PRESS [ENTER] TO START", 250, 450, 20, RED);
    }
    else if (currentState == GAME_OVER) {
        DrawText("GAME OVER", 300, 250, 40, RED);
        DrawText("PRESS [R] TO RESTART", 280, 320, 20, DARKGRAY);
    }
    else if (currentState == WIN) {
        DrawText("YOU WIN!", 320, 250, 40, GREEN);
        DrawText("PRESS [R] TO RETURN TO MENU", 240, 320, 20, DARKGRAY);
    }
    else {
        // Gameplay Drawing
        
        // Draw Platforms
        for (const auto& plat : platforms) {
            DrawRectangleRec(plat.rect, plat.color);
        }

        // Draw Player
        DrawRectangleRec(player.rect, BLUE);
        DrawText("Player", player.rect.x, player.rect.y - 20, 10, BLUE);

        // Draw Enemies
        for (const auto& e : enemies) {
            if (e.active) {
                Color enemyColor = (e.type == Enemy::PATROL) ? RED : ORANGE;
                DrawRectangleRec(e.rect, enemyColor);
                DrawText(e.type == Enemy::PATROL ? "Patrol" : "Chase", e.rect.x, e.rect.y - 10, 10, enemyColor);
            }
        }

        // Draw Bullets
        for (const auto& b : bullets) {
            if (b.active) DrawCircleV(b.position, b.radius, BLACK);
        }

        // UI
        DrawText("Controls: Arrows to Move, Space to Jump, Z to Shoot", 10, 10, 20, LIGHTGRAY);
        DrawText(TextFormat("Level: %d", (currentState == LEVEL_1 ? 1 : 2)), 10, 40, 20, BLACK);
    }

    EndDrawing();
}