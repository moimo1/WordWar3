#ifndef UIMANAGER_H
#define UIMANAGER_H

#include "raylib.h"
#include <string>

class UIManager {
private:
    int logicalWidth;
    int logicalHeight;
    RenderTexture2D target;

    // Helper functions for drawing layout sections
    void drawTopPanel();
    void drawWordPool();
    void drawCombatLog();
    void drawSentenceBuilder();

public:
    UIManager(int width, int height, const std::string& title);
    ~UIManager();

    void updateAndDraw();
    bool shouldClose() const;
};

#endif // UIMANAGER_H