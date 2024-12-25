#include "raylib.h"
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>

// Constants
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int CELL_SIZE = 8;  // Smaller cells for more detail
const int GRID_WIDTH = SCREEN_WIDTH / CELL_SIZE;
const int GRID_HEIGHT = SCREEN_HEIGHT / CELL_SIZE;
const float TEMPERATURE_SPREAD = 0.2f;
const float COOLING_RATE = 0.05f;
const float GRAVITY = 0.6f;

// Particle types
enum class ParticleType {
    EMPTY,
    SAND,
    WATER,
    WALL,
    FIRE,
    SMOKE,
    STEAM,
    LAVA,
    ICE,
    OIL,
    ACID,
    WOOD,
    PLANT,
    SALT,
    GLASS,
    METAL
};

// Particle properties
struct ParticleProperties {
    Color color;
    bool movable;
    bool flammable;
    float mass;
    float temperature;
    float conductivity;
    float viscosity;
    int lifetime;
};

// Particle struct
struct Particle {
    ParticleType type = ParticleType::EMPTY;
    Color color = BLACK;
    bool updated = false;
    float temperature = 20.0f;  // Temperature in Celsius
    float velocity_y = 0.0f;
    float velocity_x = 0.0f;
    int lifetime = -1;  // -1 for infinite
};

// Global variables
std::vector<std::vector<Particle>> grid(GRID_WIDTH, std::vector<Particle>(GRID_HEIGHT));
std::vector<std::vector<Particle>> next_grid(GRID_WIDTH, std::vector<Particle>(GRID_HEIGHT));
ParticleType currentType = ParticleType::SAND;
int brushSize = 3;
bool paused = false;
bool showDebug = false;
float gravity = 0.5f;
Vector2 wind = {0.0f, 0.0f};
bool symmetryMode = false;
Camera2D camera = {0};

// Particle properties lookup table
std::unordered_map<ParticleType, ParticleProperties> particleProps = {
    {ParticleType::SAND, {GOLD, true, false, 1.5f, 20.0f, 0.2f, 0.0f, -1}},
    {ParticleType::WATER, {BLUE, true, false, 1.0f, 20.0f, 0.5f, 0.8f, -1}},
    {ParticleType::WALL, {DARKGRAY, false, false, 999.0f, 20.0f, 0.1f, 0.0f, -1}},
    {ParticleType::FIRE, {RED, true, false, 0.1f, 800.0f, 1.0f, 0.0f, 100}},
    {ParticleType::SMOKE, {DARKGRAY, true, false, 0.2f, 100.0f, 0.1f, 0.3f, 200}},
    {ParticleType::STEAM, {LIGHTGRAY, true, false, 0.3f, 100.0f, 0.3f, 0.2f, 150}},
    {ParticleType::LAVA, {ORANGE, true, false, 2.0f, 1000.0f, 0.8f, 0.9f, -1}},
    {ParticleType::ICE, {SKYBLUE, false, false, 0.9f, -10.0f, 0.9f, 0.0f, -1}},
    {ParticleType::OIL, {BROWN, true, true, 0.8f, 20.0f, 0.1f, 0.4f, -1}},
    {ParticleType::ACID, {GREEN, true, false, 1.2f, 20.0f, 0.3f, 0.5f, -1}},
    {ParticleType::WOOD, {BEIGE, false, true, 0.7f, 20.0f, 0.2f, 0.0f, -1}},
    {ParticleType::PLANT, {DARKGREEN, false, true, 0.6f, 20.0f, 0.3f, 0.0f, -1}},
    {ParticleType::SALT, {WHITE, true, false, 1.1f, 20.0f, 0.2f, 0.0f, -1}},
    {ParticleType::GLASS, {Fade(WHITE, 0.5f), false, false, 1.5f, 20.0f, 0.4f, 0.0f, -1}},
    {ParticleType::METAL, {LIGHTGRAY, false, false, 2.0f, 20.0f, 0.9f, 0.0f, -1}}
};

// Function declarations
void InitializeGrid();
void UpdateParticles();
void DrawGrid();
void HandleInput();
void UpdateParticle(int x, int y);
bool IsValidPosition(int x, int y);
void PlaceParticles(int x, int y, ParticleType type);
void UpdateTemperature(int x, int y);
void HandleParticleInteractions(int x, int y);
void ProcessChemicalReactions(int x, int y);
Color GetTemperatureColor(Color baseColor, float temperature);
void DrawUI();
void SaveToFile(const char* filename);
void LoadFromFile(const char* filename);
void UpdatePhysics(int x, int y);

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenBox");
    SetTargetFPS(60);

    // Initialize camera
    camera.zoom = 1.0f;
    camera.target = {0, 0};
    camera.offset = {0, 0};  // Start at top-left corner
    camera.rotation = 0.0f;

    Image icon = LoadImage("resources/icon.png");
    SetWindowIcon(icon);

    InitializeGrid();

    while (!WindowShouldClose()) {
        HandleInput();
        
        if (!paused) {
            UpdateParticles();
        }

        BeginDrawing();
        ClearBackground(BLACK);
        
        BeginMode2D(camera);
        DrawGrid();
        EndMode2D();
        
        DrawUI();
        EndDrawing();
    }

    CloseWindow();
    UnloadImage(icon);
    return 0;
}

void InitializeGrid() {
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            grid[x][y].type = ParticleType::EMPTY;
            grid[x][y].color = BLACK;
            grid[x][y].temperature = 20.0f;
            grid[x][y].velocity_x = 0.0f;
            grid[x][y].velocity_y = 0.0f;
            grid[x][y].lifetime = -1;
            grid[x][y].updated = false;
            
            // Create floor at the bottom
            if (y == GRID_HEIGHT - 1) {
                grid[x][y].type = ParticleType::WALL;
                grid[x][y].color = particleProps[ParticleType::WALL].color;
            }
            
            // Create walls at the sides
            if (x == 0 || x == GRID_WIDTH - 1) {
                grid[x][y].type = ParticleType::WALL;
                grid[x][y].color = particleProps[ParticleType::WALL].color;
            }
        }
    }
}

void UpdateParticles() {
    // Reset updated flag
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            grid[x][y].updated = false;
        }
    }

    // Update particles from bottom to top
    for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            UpdateParticle(x, y);
        }
    }
}

bool IsValidPosition(int x, int y) {
    return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
}

void PlaceParticles(int x, int y, ParticleType type) {
    if (!IsValidPosition(x, y)) return;

    for (int dx = -brushSize; dx <= brushSize; dx++) {
        for (int dy = -brushSize; dy <= brushSize; dy++) {
            int newX = x + dx;
            int newY = y + dy;
            
            if (IsValidPosition(newX, newY)) {
                float distance = sqrt(dx*dx + dy*dy);
                if (distance <= brushSize) {
                    grid[newX][newY].type = type;
                    grid[newX][newY].color = particleProps[type].color;
                    grid[newX][newY].temperature = particleProps[type].temperature;
                    grid[newX][newY].velocity_x = 0.0f;
                    grid[newX][newY].velocity_y = 0.0f;
                    grid[newX][newY].lifetime = particleProps[type].lifetime;
                    grid[newX][newY].updated = false;
                }
            }
        }
    }
}

void UpdatePhysics(int x, int y) {
    Particle& current = grid[x][y];
    
    if (!particleProps[current.type].movable) return;

    bool moved = false;
    
    // Try to move down first
    if (y < GRID_HEIGHT - 1 && grid[x][y + 1].type == ParticleType::EMPTY) {
        std::swap(grid[x][y], grid[x][y + 1]);
        moved = true;
    } 
    // If can't move down, and it's water, try to spread sideways
    else if (current.type == ParticleType::WATER || current.type == ParticleType::OIL) {
        // Randomly try left or right first
        int dir = (GetRandomValue(0, 1) * 2 - 1);  // -1 or 1
        
        // Try first direction
        if (IsValidPosition(x + dir, y) && grid[x + dir][y].type == ParticleType::EMPTY) {
            std::swap(grid[x][y], grid[x + dir][y]);
            moved = true;
        }
        // Try other direction
        else if (IsValidPosition(x - dir, y) && grid[x - dir][y].type == ParticleType::EMPTY) {
            std::swap(grid[x][y], grid[x - dir][y]);
            moved = true;
        }
    }
    // For sand, try to move diagonally down if blocked
    else if (current.type == ParticleType::SAND) {
        // Randomly try left or right first
        int dir = (GetRandomValue(0, 1) * 2 - 1);  // -1 or 1
        
        if (IsValidPosition(x + dir, y + 1) && grid[x + dir][y + 1].type == ParticleType::EMPTY) {
            std::swap(grid[x][y], grid[x + dir][y + 1]);
            moved = true;
        }
        else if (IsValidPosition(x - dir, y + 1) && grid[x - dir][y + 1].type == ParticleType::EMPTY) {
            std::swap(grid[x][y], grid[x - dir][y + 1]);
            moved = true;
        }
    }
    
    // Reset velocities since we're not using them anymore
    if (moved) {
        current.velocity_x = 0.0f;
        current.velocity_y = 0.0f;
    }
}

void UpdateParticle(int x, int y) {
    if (!IsValidPosition(x, y) || grid[x][y].updated) return;

    Particle& current = grid[x][y];
    current.updated = true;

    // Update lifetime
    if (current.lifetime > 0) {
        current.lifetime--;
        if (current.lifetime <= 0) {
            current.type = ParticleType::EMPTY;
            return;
        }
    }

    // Update physics
    UpdatePhysics(x, y);
    
    // Update temperature
    UpdateTemperature(x, y);
    
    // Handle particle-specific behavior
    HandleParticleInteractions(x, y);
    
    // Process chemical reactions
    ProcessChemicalReactions(x, y);
}

void HandleParticleInteractions(int x, int y) {
    Particle& current = grid[x][y];
    
    switch (current.type) {
        case ParticleType::WATER: {
            // Water extinguishes fire
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (IsValidPosition(x + dx, y + dy) && 
                        grid[x + dx][y + dy].type == ParticleType::FIRE) {
                        grid[x + dx][y + dy].type = ParticleType::STEAM;
                    }
                }
            }
            
            // Water freezes below 0°C
            if (current.temperature < 0) {
                current.type = ParticleType::ICE;
            }
            break;
        }
        
        case ParticleType::FIRE: {
            // Fire spreads to flammable materials
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (IsValidPosition(x + dx, y + dy)) {
                        ParticleType nearbyType = grid[x + dx][y + dy].type;
                        if (particleProps[nearbyType].flammable) {
                            if (GetRandomValue(0, 100) < 10) {
                                grid[x + dx][y + dy].type = ParticleType::FIRE;
                            }
                        }
                    }
                }
            }
            
            // Create smoke
            if (GetRandomValue(0, 100) < 5) {
                if (IsValidPosition(x, y - 1) && grid[x][y - 1].type == ParticleType::EMPTY) {
                    grid[x][y - 1].type = ParticleType::SMOKE;
                }
            }
            break;
        }
        
        case ParticleType::LAVA: {
            // Lava turns water to steam
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (IsValidPosition(x + dx, y + dy)) {
                        if (grid[x + dx][y + dy].type == ParticleType::WATER) {
                            grid[x + dx][y + dy].type = ParticleType::STEAM;
                        }
                    }
                }
            }
            
            // Lava cools to metal
            if (current.temperature < 800) {
                current.type = ParticleType::METAL;
            }
            break;
        }
        
        case ParticleType::ACID: {
            // Acid dissolves materials except glass
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (IsValidPosition(x + dx, y + dy)) {
                        ParticleType nearbyType = grid[x + dx][y + dy].type;
                        if (nearbyType != ParticleType::EMPTY && 
                            nearbyType != ParticleType::ACID &&
                            nearbyType != ParticleType::GLASS) {
                            if (GetRandomValue(0, 100) < 20) {
                                grid[x + dx][y + dy].type = ParticleType::EMPTY;
                            }
                        }
                    }
                }
            }
            break;
        }
        
        default:
            break;
    }
}

void ProcessChemicalReactions(int x, int y) {
    Particle& current = grid[x][y];
    
    // Sand + Heat = Glass
    if (current.type == ParticleType::SAND && current.temperature > 1700) {
        current.type = ParticleType::GLASS;
        return;
    }
    
    // Water + Salt = Salt Water (dissolves salt)
    if (current.type == ParticleType::WATER) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (IsValidPosition(x + dx, y + dy) && 
                    grid[x + dx][y + dy].type == ParticleType::SALT) {
                    grid[x + dx][y + dy].type = ParticleType::EMPTY;
                    current.color = SKYBLUE;  // Change water color to indicate salt water
                }
            }
        }
    }
}

void UpdateTemperature(int x, int y) {
    Particle& current = grid[x][y];
    
    // Temperature spreading
    float avgTemp = current.temperature;
    int count = 1;
    
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (IsValidPosition(x + dx, y + dy)) {
                avgTemp += grid[x + dx][y + dy].temperature;
                count++;
            }
        }
    }
    
    avgTemp /= count;
    current.temperature = avgTemp * TEMPERATURE_SPREAD + 
                         current.temperature * (1 - TEMPERATURE_SPREAD);
    
    // Natural cooling
    if (current.temperature > 20) {
        current.temperature -= COOLING_RATE;
    } else if (current.temperature < 20) {
        current.temperature += COOLING_RATE;
    }
}

Color GetTemperatureColor(Color baseColor, float temperature) {
    if (temperature > 100) {
        // Add red tint for hot particles
        return Color{
            (unsigned char)std::min(255, baseColor.r + (int)((temperature - 100) / 4)),
            (unsigned char)std::max(0, baseColor.g - (int)((temperature - 100) / 8)),
            (unsigned char)std::max(0, baseColor.b - (int)((temperature - 100) / 8)),
            baseColor.a
        };
    } else if (temperature < 0) {
        // Add blue tint for cold particles
        return Color{
            (unsigned char)std::max(0, baseColor.r - (int)((-temperature) / 8)),
            (unsigned char)std::max(0, baseColor.g - (int)((-temperature) / 8)),
            (unsigned char)std::min(255, baseColor.b + (int)((-temperature) / 4)),
            baseColor.a
        };
    }
    return baseColor;
}

std::string GetParticleTypeName(ParticleType type) {
    switch(type) {
        case ParticleType::SAND: return "Sand";
        case ParticleType::WATER: return "Water";
        case ParticleType::WALL: return "Wall";
        case ParticleType::FIRE: return "Fire";
        case ParticleType::SMOKE: return "Smoke";
        case ParticleType::STEAM: return "Steam";
        case ParticleType::LAVA: return "Lava";
        case ParticleType::ICE: return "Ice";
        case ParticleType::OIL: return "Oil";
        case ParticleType::ACID: return "Acid";
        case ParticleType::WOOD: return "Wood";
        case ParticleType::PLANT: return "Plant";
        case ParticleType::SALT: return "Salt";
        case ParticleType::GLASS: return "Glass";
        case ParticleType::METAL: return "Metal";
        default: return "Empty";
    }
}

void DrawUI() {
    const int lineHeight = 25;
    int currentY = 10;
    
    // Draw current tool info
    DrawText(TextFormat("Particle Type: %s", GetParticleTypeName(currentType).c_str()), 10, currentY, 20, WHITE);
    currentY += lineHeight;
    
    DrawText(TextFormat("Brush Size: %d", brushSize), 10, currentY, 20, WHITE);
    currentY += lineHeight;
    
    // Add bounds checking for mouse position
    Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
    int gridX = mousePos.x / CELL_SIZE;
    int gridY = mousePos.y / CELL_SIZE;
    
    if (IsValidPosition(gridX, gridY)) {
        DrawText(TextFormat("Temperature: %.1f°C", grid[gridX][gridY].temperature), 10, currentY, 20, WHITE);
    } else {
        DrawText("Temperature: --°C", 10, currentY, 20, WHITE);
    }
    currentY += lineHeight;
    
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, currentY, 20, WHITE);
    currentY += lineHeight;
    
    if (paused) {
        DrawText("PAUSED", SCREEN_WIDTH/2 - 50, 10, 30, RED);
    }
    
    // Draw controls help
    currentY = SCREEN_HEIGHT - 220;
    DrawText("Controls:", 10, currentY, 20, WHITE);
    currentY += lineHeight;
    DrawText("1-9: Select particle type", 10, currentY, 20, WHITE);
    currentY += lineHeight;
    DrawText("[/]: Adjust brush size", 10, currentY, 20, WHITE);
    currentY += lineHeight;
    DrawText("Space: Pause/Resume", 10, currentY, 20, WHITE);
    currentY += lineHeight;
    DrawText("R: Reset simulation", 10, currentY, 20, WHITE);
    currentY += lineHeight;
    DrawText("M: Symmetry Mode", 10, currentY, 20, WHITE);
    currentY += lineHeight;
    DrawText("S: Save Simulation to File", 10, currentY, 20, WHITE);
    currentY += lineHeight;
    DrawText("L: Load Simulation from File", 10, currentY, 20, WHITE);
}

void HandleInput() {
    // Camera controls
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            camera.target.x -= delta.x;
            camera.target.y -= delta.y;
        }
        
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            camera.zoom += wheel * 0.05f;
            if (camera.zoom < 0.1f) camera.zoom = 0.1f;
            if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        }
    }

    // Particle type selection
    if (IsKeyPressed(KEY_ONE)) currentType = ParticleType::SAND;
    if (IsKeyPressed(KEY_TWO)) currentType = ParticleType::WATER;
    if (IsKeyPressed(KEY_THREE)) currentType = ParticleType::WALL;
    if (IsKeyPressed(KEY_FOUR)) currentType = ParticleType::FIRE;
    if (IsKeyPressed(KEY_FIVE)) currentType = ParticleType::LAVA;
    if (IsKeyPressed(KEY_SIX)) currentType = ParticleType::ICE;
    if (IsKeyPressed(KEY_SEVEN)) currentType = ParticleType::OIL;
    if (IsKeyPressed(KEY_EIGHT)) currentType = ParticleType::ACID;
    if (IsKeyPressed(KEY_NINE)) currentType = ParticleType::WOOD;
    
    // Brush size
    if (IsKeyPressed(KEY_LEFT_BRACKET)) brushSize = std::max(1, brushSize - 1);
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) brushSize = std::min(20, brushSize + 1);
    
    // Simulation controls
    if (IsKeyPressed(KEY_SPACE)) paused = !paused;
    if (IsKeyPressed(KEY_R)) InitializeGrid();
    if (IsKeyPressed(KEY_S)) SaveToFile("sandbox_save.dat");
    if (IsKeyPressed(KEY_L)) LoadFromFile("sandbox_save.dat");
    if (IsKeyPressed(KEY_M)) symmetryMode = !symmetryMode;
    
    // Debug toggle
    if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;
    
    // Place particles with mouse
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
        int x = mousePos.x / CELL_SIZE;
        int y = mousePos.y / CELL_SIZE;
        
        PlaceParticles(x, y, currentType);
        if (symmetryMode) {
            // Place particles symmetrically
            PlaceParticles(GRID_WIDTH - 1 - x, y, currentType);
        }
    }
    
    // Erase particles with right mouse button
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
        Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
        int x = mousePos.x / CELL_SIZE;
        int y = mousePos.y / CELL_SIZE;
        
        PlaceParticles(x, y, ParticleType::EMPTY);
        if (symmetryMode) {
            PlaceParticles(GRID_WIDTH - 1 - x, y, ParticleType::EMPTY);
        }
    }
    
    // Wind control
    if (IsKeyDown(KEY_LEFT)) wind.x = -0.1f;
    else if (IsKeyDown(KEY_RIGHT)) wind.x = 0.1f;
    else wind.x = 0.0f;
    
    if (IsKeyDown(KEY_UP)) wind.y = -0.1f;
    else if (IsKeyDown(KEY_DOWN)) wind.y = 0.1f;
    else wind.y = 0.0f;
}

void SaveToFile(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            for (int y = 0; y < GRID_HEIGHT; y++) {
                fwrite(&grid[x][y], sizeof(Particle), 1, file);
            }
        }
        fclose(file);
    }
}

void LoadFromFile(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            for (int y = 0; y < GRID_HEIGHT; y++) {
                fread(&grid[x][y], sizeof(Particle), 1, file);
            }
        }
        fclose(file);
    }
}

void DrawGrid() {
    int startX = std::max(0, static_cast<int>(-camera.target.x / CELL_SIZE));
    int startY = std::max(0, static_cast<int>(-camera.target.y / CELL_SIZE));
    int endX = std::min(GRID_WIDTH, static_cast<int>((SCREEN_WIDTH - camera.target.x) / CELL_SIZE + 1));
    int endY = std::min(GRID_HEIGHT, static_cast<int>((SCREEN_HEIGHT - camera.target.y) / CELL_SIZE + 1));

    for (int x = startX; x < endX; x++) {
        for (int y = startY; y < endY; y++) {
            if (grid[x][y].type != ParticleType::EMPTY) {
                Color particleColor = particleProps[grid[x][y].type].color;
                particleColor = GetTemperatureColor(particleColor, grid[x][y].temperature);
                
                DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, 
                             CELL_SIZE, CELL_SIZE, particleColor);
                             
                if (showDebug && grid[x][y].velocity_y != 0) {
                    DrawLine(x * CELL_SIZE + CELL_SIZE/2, 
                            y * CELL_SIZE + CELL_SIZE/2,
                            x * CELL_SIZE + CELL_SIZE/2 + grid[x][y].velocity_x * 5,
                            y * CELL_SIZE + CELL_SIZE/2 + grid[x][y].velocity_y * 5,
                            RED);
                }
            }
        }
    }
}