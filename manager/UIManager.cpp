#include "UIManager.h"
#include <algorithm>

UIManager::UIManager(int width, int height, const std::string& title) 
    : logicalWidth(width), logicalHeight(height) 
{
    // Configure window to be resizable and try to lock to vsync
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(width, height, title.c_str());
    
    // Switch to borderless fullscreen matching the native resolution
    int monitor = GetCurrentMonitor();
    SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
    ToggleFullscreen();

    // Setup an internal hidden canvas matching our strict 1280x720 aspect ratio
    target = LoadRenderTexture(logicalWidth, logicalHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    SetTargetFPS(60);
}

UIManager::~UIManager() {
    UnloadRenderTexture(target);
    CloseWindow();
}

bool UIManager::shouldClose() const {
    return WindowShouldClose();
}

// Helper to draw a modern panel with rounded corners
static void drawPanel(float x, float y, float width, float height, const char* text, int fontSize) {
    Rectangle rec = { x, y, width, height };
    DrawRectangleRounded(rec, 0.1f, 10, Color{230, 230, 230, 255});
    
    if (text != nullptr) {
        int textWidth = MeasureText(text, fontSize);
        DrawText(text, x + width / 2 - textWidth / 2, y + height / 2 - fontSize / 2, fontSize, Color{40, 40, 40, 255});
    }
}

void UIManager::updateAndDraw() {
    // 1. Draw UI to the internal 1280x720 render target canvas
    BeginTextureMode(target);
    ClearBackground(Color{15, 35, 45, 255}); // Dark grayish blue background

    drawTopPanel();
    drawWordPool();
    drawCombatLog();
    drawSentenceBuilder();

    EndTextureMode();

    // 2. Scale up (or down) and draw the canvas to the physical monitor screen
    BeginDrawing();
    ClearBackground(BLACK); // Black borders (letterboxing) if aspect ratio doesn't strictly match

    float scale = std::min((float)GetScreenWidth() / logicalWidth, (float)GetScreenHeight() / logicalHeight);
    
    Rectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height }; // OpenGL trick: flip Y coordinate
    Rectangle destRec = { 
        (GetScreenWidth() - (logicalWidth * scale)) * 0.5f, 
        (GetScreenHeight() - (logicalHeight * scale)) * 0.5f, 
        logicalWidth * scale, 
        logicalHeight * scale 
    };
    
    DrawTexturePro(target.texture, sourceRec, destRec, Vector2{ 0, 0 }, 0.0f, WHITE);

    EndDrawing();
}

void UIManager::drawTopPanel() {
    float padding = 30.0f;
    float topY = 30.0f;
    float panelHeight = 80.0f;

    // Player Status
    Rectangle p1Rec = { padding, topY, 300, panelHeight };
    DrawRectangleRounded(p1Rec, 0.15f, 10, Color{230, 230, 230, 255});
    DrawText("Player", p1Rec.x + 15, p1Rec.y + 15, 20, DARKGRAY);
    DrawText("HP", p1Rec.x + 15, p1Rec.y + 45, 15, DARKGRAY);
    
    Rectangle hpBg = { p1Rec.x + 50, p1Rec.y + 45, 230, 20 };
    DrawRectangleRounded(hpBg, 0.5f, 5, DARKGRAY);
    Rectangle hpFg = { p1Rec.x + 50, p1Rec.y + 45, 180, 20 }; 
    DrawRectangleRounded(hpFg, 0.5f, 5, MAROON);

    // Round Count
    float roundWidth = 200.0f;
    Rectangle roundRec = { logicalWidth / 2.0f - roundWidth / 2.0f, topY, roundWidth, panelHeight };
    DrawRectangleRounded(roundRec, 0.15f, 10, Color{230, 230, 230, 255});
    DrawText("Round Count", roundRec.x + roundWidth/2 - MeasureText("Round Count", 20)/2, roundRec.y + panelHeight/2 - 10, 20, DARKGRAY);

    // Opponent Status
    Rectangle p2Rec = { logicalWidth - padding - 300, topY, 300, panelHeight };
    DrawRectangleRounded(p2Rec, 0.15f, 10, Color{230, 230, 230, 255});
    DrawText("Opponent", p2Rec.x + 15, p2Rec.y + 15, 20, DARKGRAY);
    DrawText("HP", p2Rec.x + 15, p2Rec.y + 45, 15, DARKGRAY);
    
    Rectangle hpBg2 = { p2Rec.x + 110, p2Rec.y + 45, 170, 20 };
    DrawRectangleRounded(hpBg2, 0.5f, 5, DARKGRAY);
    Rectangle hpFg2 = { p2Rec.x + 110, p2Rec.y + 45, 140, 20 }; 
    DrawRectangleRounded(hpFg2, 0.5f, 5, MAROON);
}

void UIManager::drawWordPool() {
    float padding = 30.0f;
    float yOffset = 140.0f;
    float wpWidth = logicalWidth * 0.45f;
    float wpHeight = 350.0f;

    drawPanel(padding, yOffset, wpWidth, wpHeight, "Word Pool", 30);
}

void UIManager::drawCombatLog() {
    float padding = 30.0f;
    float yOffset = 140.0f;
    float clWidth = logicalWidth * 0.45f;
    float clHeight = 520.0f;

    drawPanel(logicalWidth - padding - clWidth, yOffset, clWidth, clHeight, "Combat Log", 40);
}

void UIManager::drawSentenceBuilder() {
    float padding = 30.0f;
    float yOffset = 520.0f;
    float sbWidth = logicalWidth * 0.45f;
    float sbHeight = 140.0f;

    drawPanel(padding, yOffset, sbWidth, sbHeight, "Sentence Builder", 25);
}