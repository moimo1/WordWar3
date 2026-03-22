#ifndef UIMANAGER_H
#define UIMANAGER_H

#include "raylib.h"
#include <string>
#include <vector>
#include "../network/Client.h"
#include "../player/Sentence.h"
#include "../word/Word.h"

enum class UIState {
    LOGIN,
    MAIN_MENU,
    LEADERBOARD,
    WAITING,
    BATTLE,
    GAMEOVER
};

class UIManager {
private:
    int logicalWidth;
    int logicalHeight;
    RenderTexture2D target;
    
    Client* networkClient;
    UIState currentState;
    bool exitTriggered;

    std::string authUsername;
    std::string authPassword;
    bool typingPassword; 
    bool waitingForTurn; // Prevents spamming sentences after submission!
    std::string loginErrorMessage;

    std::vector<std::pair<std::string, int>> leaderboard;

    std::vector<Word> currentPool;
    Sentence currentSentence;
    std::string typingBuffer;
    std::vector<std::string> combatLog;
    int lastCombatLogSize;
    float combatLogScrollOffset;
    int playerHp, playerShield, playerMaxHp;
    int opponentHp, opponentShield, opponentMaxHp;
    std::string opponentName;
    std::string gameOverMessage;

    Vector2 getLogicalMousePos() const;
    void processNetworkPackets();
    void handleKeyboardInput();
    void handleLoginInput();

    void drawLoginScreen();
    void drawMainMenu();
    void drawLeaderboardScreen();
    void drawWaitingScreen();
    void drawGameOverScreen();

    void drawTopPanel();
    void drawWordPool();
    void drawCombatLog();
    void drawSentenceBuilder();

public:
    UIManager(int width, int height, const std::string& title, Client* client);
    ~UIManager();

    void updateAndDraw();
    bool shouldClose() const;
};

#endif // UIMANAGER_H