#include "raylib.h"
#include "raymath.h"
#include "raygui.h"

#include <cassert>
#include <array>
#include <vector>
#include <algorithm>

constexpr float SCREEN_SIZE = 800;

constexpr int TILE_COUNT = 20;
constexpr float TILE_SIZE = SCREEN_SIZE / TILE_COUNT;

enum TileType : int
{
    GRASS,      // Marks unoccupied space, can be overwritten 
    DIRT,       // Marks the path, cannot be overwritten
    WAYPOINT,   // Marks where the path turns, cannot be overwritten
    TURRET, 
    COUNT
};

struct Cell
{
    int row;
    int col;
};

constexpr std::array<Cell, 4> DIRECTIONS{ Cell{ -1, 0 }, Cell{ 1, 0 }, Cell{ 0, -1 }, Cell{ 0, 1 } };

inline bool InBounds(Cell cell, int rows = TILE_COUNT, int cols = TILE_COUNT)
{
    return cell.col >= 0 && cell.col < cols && cell.row >= 0 && cell.row < rows;
}

void DrawTile(int row, int col, Color color)
{
    DrawRectangle(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE, color);
}

void DrawTile(int row, int col, int type)
{
    Color colors[4] =
    {
        GREEN,
        BEIGE,
        SKYBLUE,
        RED
    };
    //assert(type >= 0 && type < COUNT);
    DrawTile(row, col, colors[type]);
}

Vector2 TileCenter(int row, int col)
{
    float x = col * TILE_SIZE + TILE_SIZE * 0.5f;
    float y = row * TILE_SIZE + TILE_SIZE * 0.5f;
    return { x, y };
}

// Returns a collection of adjacent cells that match the search value.
std::vector<Cell> FloodFill(Cell start, int tiles[TILE_COUNT][TILE_COUNT], TileType searchValue)
{
    // "open" = "places we want to search", "closed" = "places we've already searched".
    std::vector<Cell> result;
    std::vector<Cell> open;
    bool closed[TILE_COUNT][TILE_COUNT]{};
    for (int row = 0; row < TILE_COUNT; row++)
    {
        for (int col = 0; col < TILE_COUNT; col++)
        {
            // We don't want to search zero-tiles, so add them to closed!
            closed[row][col] = tiles[row][col] == 0;
        }
    }

    // Add the starting cell to the exploration queue & search till there's nothing left!
    open.push_back(start);
    while (!open.empty())
    {
        // Remove from queue and prevent revisiting
        Cell cell = open.back();
        open.pop_back();
        closed[cell.row][cell.col] = true;

        // Add to result if explored cell has the desired value
        if (tiles[cell.row][cell.col] == searchValue)
            result.push_back(cell);

        // Search neighbours
        for (Cell dir : DIRECTIONS)
        {
            Cell adj = { cell.row + dir.row, cell.col + dir.col };
            if (InBounds(adj) && !closed[adj.row][adj.col] && tiles[adj.row][adj.col] != 0)
                open.push_back(adj);
        }
    }

    return result;
}

struct Bullet
{
    Vector2 position;
    Vector2 direction;
    float speed;
    float damage = 10.0f;
    float time;
    float radius;
    Color color;
    bool destroy;
};

// Fill in for Lab 2
struct Enemy
{
    Cell cell;
    Vector2 position;
    int curr;
    int next;
    float health;
    float speed;
    float min_distance = speed / 60.0f;
    bool at_end;
    bool destroy;
};

// Fill in for Lab 2
struct Turret
{
    Cell cell;
    Enemy* target = nullptr;
    Vector2 position;
    float cooldown;
    float range;
};

int main()
{
    int tiles[TILE_COUNT][TILE_COUNT]
    {
        //col:0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19    row:
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0 }, // 0
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 1
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 2
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 3
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 4
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 5
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 6
            { 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0 }, // 7
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 8
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 9
            { 0, 0, 0, 1, 0, 3, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 10
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 11
            { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 12
            { 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0 }, // 13
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 14
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 3, 0, 1, 0, 0, 0 }, // 15
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 }, // 16
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0 }, // 17
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 18
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }  // 19
    };
    std::vector<Cell> waypoints = FloodFill({ 0, 12 }, tiles, WAYPOINT);
    int curr = 0;
    //int next = curr + 1;

    //Vector2 enemy_position = TileCenter(waypoints[curr].row, waypoints[curr].col);
    //float enemy_speed = 250.0f;   // <-- 250 pixels per second
    //bool at_end = false;

    float enemy_spawn_timer = 0.0f;
    int enemy_spawn_total = 10;
    int enemy_spawn_count = 0;

    float bullet_time_current = 0.0f;
    float bullet_time_total = 0.25f;
    std::vector<Bullet> bullets;

    std::vector<Turret> turrets; // Use in Assignment 1
    std::vector<Enemy> enemies;  // Use in Lab 2 and Assignment 1

    // Search array for desired tiles (ie turrets)
    for (int row = 0; row < TILE_COUNT; row++)
    {
        for (int col = 0; col < TILE_COUNT; col++)
        {
            if (tiles[row][col] == TURRET)
            {
                // Construct turret here
                Turret turret{};
                turret.position = TileCenter(row, col);
                turret.cooldown = 0.0f;
                turret.range = 300.0f;
                turret.cell = { row, col };
                turrets.push_back(turret);
            }
        }
    }

    InitWindow(SCREEN_SIZE, SCREEN_SIZE, "Tower Defense");
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        enemy_spawn_timer += dt;
        if (enemy_spawn_timer >= 1.0f && enemy_spawn_count < enemy_spawn_total)
        {
            Enemy enemy{};
            enemy.curr = 0;
            enemy.next = 1;
            enemy.cell = waypoints[0];
            enemy.position = TileCenter(waypoints[0].row, waypoints[0].col);

            enemy.health = 100.0f;
            enemy.speed = 250.0f;
			enemy.min_distance = enemy.speed / 60.0f;

            enemy.destroy = false;
            enemy.at_end = false;

            enemies.push_back(enemy);

            enemy_spawn_timer = 0.0f;
            enemy_spawn_count++;
        }

        // Enemy movement (need to loop through enemies vector to get marks on Lab 2 and Assignment 1)
        for (Enemy& enemy : enemies)
        {
			// Destroy enemy if health is <= 0.0f
            if (enemy.health <= 0.0f)
            {
				enemy.destroy = true;
            }

            // Lift this movement logic to the above loop to make a generalized enemy system
            if (!enemy.at_end)
            {
                Vector2 from = TileCenter(waypoints[enemy.curr].row, waypoints[enemy.curr].col);
                Vector2 to = TileCenter(waypoints[enemy.next].row, waypoints[enemy.next].col);
                Vector2 direction = Vector2Normalize(to - from);
                enemy.position += direction * enemy.speed * dt;

                // Tolorance depends on enemy speed
                if (CheckCollisionPointCircle(enemy.position, to, enemy.min_distance))
                {
                    enemy.position = to;

                    enemy.curr++;
                    enemy.next++;
                    enemy.at_end = enemy.curr >= waypoints.size() - 1;
                }
            }
        }

        // Acquire nearest enemy as target and shoot it if the turret is off-cooldown
        for (Turret& turret : turrets)
        {
            float max = FLT_MAX;
            for (Enemy& enemy : enemies)
            {
                float distance = Vector2Distance(turret.position, enemy.position);
                if (distance < turret.range)
                {
                    turret.target = &enemy;
                    // Target nearest enemy
                }
            }

            turret.cooldown -= dt;
            if (turret.cooldown <= 0.0f && turret.target != nullptr)
            {
                Bullet bullet{};
                bullet.position = turret.position;
                bullet.direction = Vector2Normalize(turret.target->position - bullet.position);

                bullet.speed = 1222.0f;
                bullet.radius = 10.0f;

                bullet.color = RED;

                bullets.push_back(bullet);
                turret.cooldown = 0.5f;
            }
        }

        // Bullet update
        for (Bullet& bullet : bullets)
        {
            Vector2 bullet_velocity = bullet.direction * bullet.speed;
            bullet.position += bullet_velocity * dt;
            bullet.time += dt;

            // "Flag the current bullet to be destroyed if its been alive for 1 or more seconds"
            bullet.destroy |= bullet.time >= 1.0f;

            Rectangle screen_rec;
            screen_rec.x = 0.0f;
            screen_rec.y = 0.0f;
            screen_rec.width = GetScreenWidth();
            screen_rec.height = GetScreenHeight();

        }

        for (Bullet& bullet : bullets)
        {
            for (Enemy& enemy : enemies)
            {
                if (CheckCollisionCircles(bullet.position, bullet.radius, enemy.position, 20.0f))
                {
                    enemy.health -= bullet.damage;
                    bullet.destroy = true;

                    break;
                }
            }
        }

        std::erase_if(bullets, [](Bullet& bullet)
            {
                return bullet.destroy;
            });

        std::erase_if(enemies, [](Enemy& enemy)
            {
                return enemy.at_end || enemy.health <= 0.0f;
            });

        BeginDrawing();
        ClearBackground(BLACK);

        // Draw entire grid
        for (int row = 0; row < TILE_COUNT; row++)
        {
            for (int col = 0; col < TILE_COUNT; col++)
            {
                DrawTile(row, col, tiles[row][col]);
            }
        }

        for (const Bullet& bullet : bullets)
        {
            DrawCircleV(bullet.position, bullet.radius, bullet.color);
        }

        for (const Enemy& enemy : enemies)
        {
            DrawCircleV(enemy.position, 20.0f, GOLD);
        }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
