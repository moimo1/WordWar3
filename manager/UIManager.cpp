#include "UIManager.h"
#include <algorithm>
#include <sstream>

UIManager::UIManager(int width, int height, const std::string& title, Client* client) 
    : logicalWidth(width), logicalHeight(height), networkClient(client),
      currentState(UIState::LOGIN), exitTriggered(false), typingPassword(false), waitingForTurn(false),
      playerHp(200), playerShield(0), playerMaxHp(200), 
      opponentHp(200), opponentShield(0), opponentMaxHp(200),
      opponentName("Opponent"), gameOverMessage("")
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(width, height, title.c_str());
    
    int monitor = GetCurrentMonitor();
    SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
    ToggleFullscreen();

    target = LoadRenderTexture(logicalWidth, logicalHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
    SetTargetFPS(60);
    
    combatLog.push_back("SYSTEM: Connected. Awaiting Game Server initialization...");
}

UIManager::~UIManager() {
    UnloadRenderTexture(target);
    CloseWindow();
}

bool UIManager::shouldClose() const { return exitTriggered || WindowShouldClose(); }

Vector2 UIManager::getLogicalMousePos() const {
    float scale = std::min((float)GetScreenWidth() / logicalWidth, (float)GetScreenHeight() / logicalHeight);
    Vector2 mouse = GetMousePosition();
    Vector2 virtualMouse = { 0 };
    virtualMouse.x = (mouse.x - (GetScreenWidth() - (logicalWidth * scale))*0.5f) / scale;
    virtualMouse.y = (mouse.y - (GetScreenHeight() - (logicalHeight * scale))*0.5f) / scale;
    return virtualMouse;
}

void UIManager::processNetworkPackets() {
    while (networkClient->hasPacket()) {
        Packet p = networkClient->popPacket();
        
        if (p.type == PacketType::SERVER_LOGIN_RESPONSE) {
            bool success = p.payload["success"].get<bool>();
            std::string msg = p.payload["msg"].get<std::string>();
            if (success) {
                currentState = UIState::MAIN_MENU;
                loginErrorMessage = "";
            } else {
                loginErrorMessage = msg;
            }
        }
        else if (p.type == PacketType::SERVER_SEND_LEADERBOARD) {
            leaderboard.clear();
            for (auto& item : p.payload) {
                leaderboard.push_back({item["username"].get<std::string>(), item["wins"].get<int>()});
            }
        }
        else if (p.type == PacketType::SERVER_MATCH_FOUND) {
            opponentName = p.payload["opponent"].get<std::string>();
            if (p.payload.contains("my_max_hp")) {
                playerMaxHp = p.payload["my_max_hp"].get<int>();
                opponentMaxHp = p.payload["opp_max_hp"].get<int>();
                playerHp = playerMaxHp;
                opponentHp = opponentMaxHp;
                playerShield = 0;
                opponentShield = 0;
            }
        }
        else if (p.type == PacketType::SERVER_SEND_POOL) {
            currentState = UIState::BATTLE;
            waitingForTurn = false; // Safely unlock keyboard logic explicitly!
            currentPool.clear();
            for (auto& jsonItem : p.payload) {
                currentPool.push_back(Word::fromJson(jsonItem));
            }
            combatLog.push_back("Turn started! Words arrived. Start typing!");
            typingBuffer.clear();
        }
        else if (p.type == PacketType::SERVER_UPDATE_HP) {
            playerHp = p.payload["my_hp"].get<int>();
            playerShield = p.payload["my_shield"].get<int>();
            opponentHp = p.payload["opp_hp"].get<int>();
            opponentShield = p.payload["opp_shield"].get<int>();
            combatLog.push_back("Combat resolved! HP Updated.");
        }
        else if (p.type == PacketType::GAME_OVER) {
            gameOverMessage = p.payload.get<std::string>();
            currentState = UIState::GAMEOVER;
        }
        else if (p.type == PacketType::SERVER_LOG_MESSAGE) {
            combatLog.push_back(p.payload.get<std::string>());
        }
    }
}

void UIManager::handleLoginInput() {
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125)) {
            if (typingPassword) authPassword += (char)key;
            else authUsername += (char)key;
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (typingPassword && !authPassword.empty()) authPassword.pop_back();
        else if (!typingPassword && !authUsername.empty()) authUsername.pop_back();
    }

    if (IsKeyPressed(KEY_TAB)) {
        typingPassword = !typingPassword;
    }
}

void UIManager::handleKeyboardInput() {
    if (waitingForTurn) return; // Physically block Keyboard character arrays until Server clears the lock

    int key = GetCharPressed();
    while (key > 0) {
        if ((key > 32) && (key <= 125)) {
            typingBuffer += (char)key;
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (!typingBuffer.empty()) typingBuffer.pop_back();
    }

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
        if (!typingBuffer.empty()) {
            std::string wordToFind = typingBuffer;
            bool found = false;
            for (size_t i = 0; i < currentPool.size(); ++i) {
                if (currentPool[i].getText() == wordToFind) {
                    currentSentence.addWord(currentPool[i]);
                    currentPool.erase(currentPool.begin() + i);
                    found = true;
                    break;
                }
            }
            typingBuffer.clear();
        } else if (IsKeyPressed(KEY_ENTER)) { 
            if (!currentSentence.getWords().empty()) {
                nlohmann::json payload = nlohmann::json::array();
                for(const auto& w : currentSentence.getWords()) {
                    payload.push_back(w.toJson());
                }
                networkClient->sendPacket(Packet{PacketType::CLIENT_SUBMIT_TURN, payload});
                combatLog.push_back("-> Turn submitted! Waiting for server...");
                currentSentence.clear();
                typingBuffer.clear();
                waitingForTurn = true;
            }
        }
    }
}

static void drawPanel(float x, float y, float width, float height, const char* text, int fontSize) {
    Rectangle rec = { x, y, width, height };
    DrawRectangleRounded(rec, 0.1f, 10, Color{230, 230, 230, 255});
    if (text != nullptr) {
        DrawText(text, x + width / 2 - MeasureText(text, fontSize) / 2, y + 10, fontSize, Color{40, 40, 40, 255});
    }
}

static Color getEffectColor(EffectType type) {
    switch(type) {
        case EffectType::Damage: return Color{240, 90, 90, 255};   
        case EffectType::Defense: return Color{90, 180, 240, 255};  
        case EffectType::Heal: return Color{110, 220, 110, 255};    
        case EffectType::Critical: return Color{240, 160, 60, 255}; 
        case EffectType::Weaken: return Color{180, 110, 220, 255};  
        case EffectType::Amplify: return Color{250, 220, 80, 255};  
        default: return LIGHTGRAY;
    }
}

void UIManager::drawLoginScreen() {
    float cx = logicalWidth / 2.0f;
    float cy = logicalHeight / 2.0f;
    
    drawPanel(cx - 200, cy - 150, 400, 300, "WORDWAR3 TERMINAL", 25);
    
    DrawText("USERNAME", cx - 180, cy - 80, 15, DARKGRAY);
    Rectangle uRec = { cx - 180, cy - 60, 360, 40 };
    DrawRectangleRounded(uRec, 0.2f, 5, !typingPassword ? LIGHTGRAY : WHITE);
    DrawText(authUsername.c_str(), uRec.x + 10, uRec.y + 10, 20, BLACK);
    if (!typingPassword && ((int)(GetTime() * 2) % 2 == 0)) {
        DrawText("_", uRec.x + 10 + MeasureText(authUsername.c_str(), 20), uRec.y + 10, 20, MAROON);
    }
    
    DrawText("PASSWORD", cx - 180, cy - 10, 15, DARKGRAY);
    Rectangle pRec = { cx - 180, cy + 10, 360, 40 };
    DrawRectangleRounded(pRec, 0.2f, 5, typingPassword ? LIGHTGRAY : WHITE);
    
    std::string hiddenPass(authPassword.length(), '*');
    DrawText(hiddenPass.c_str(), pRec.x + 10, pRec.y + 10, 20, BLACK);
    if (typingPassword && ((int)(GetTime() * 2) % 2 == 0)) {
        DrawText("_", pRec.x + 10 + MeasureText(hiddenPass.c_str(), 20), pRec.y + 10, 20, MAROON);
    }

    DrawText("[TAB] to switch input boxes.", cx - 180, cy + 65, 12, GRAY);

    Vector2 mouse = getLogicalMousePos();
    
    Rectangle loginBtn = { cx - 140, cy + 90, 120, 35 };
    bool hoverLogin = CheckCollisionPointRec(mouse, loginBtn);
    DrawRectangleRounded(loginBtn, 0.2f, 5, hoverLogin ? LIGHTGRAY : GRAY);
    DrawText("LOGIN", loginBtn.x + 60 - MeasureText("LOGIN", 15)/2, loginBtn.y + 10, 15, WHITE);

    if (hoverLogin && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !authUsername.empty() && !authPassword.empty()) {
        networkClient->sendPacket(Packet{PacketType::CLIENT_LOGIN, {{"username", authUsername}, {"password", authPassword}}});
        loginErrorMessage = "Authenticating...";
    }

    Rectangle regBtn = { cx + 20, cy + 90, 120, 35 };
    bool hoverReg = CheckCollisionPointRec(mouse, regBtn);
    DrawRectangleRounded(regBtn, 0.2f, 5, hoverReg ? LIGHTGRAY : GRAY);
    DrawText("REGISTER", regBtn.x + 60 - MeasureText("REGISTER", 15)/2, regBtn.y + 10, 15, WHITE);

    if (hoverReg && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !authUsername.empty() && !authPassword.empty()) {
        networkClient->sendPacket(Packet{PacketType::CLIENT_REGISTER, {{"username", authUsername}, {"password", authPassword}}});
        loginErrorMessage = "Registering...";
    }
    
    if (!loginErrorMessage.empty()) {
        DrawText(loginErrorMessage.c_str(), cx - MeasureText(loginErrorMessage.c_str(), 15)/2, cy + 140, 15, RED);
    }
}

void UIManager::drawMainMenu() {
    float cx = logicalWidth / 2.0f;
    float cy = logicalHeight / 2.0f;
    
    drawPanel(cx - 150, cy - 200, 300, 400, "MAIN MENU", 30);
    
    Vector2 mouse = getLogicalMousePos();
    
    // Play Button
    Rectangle playRec = { cx - 100, cy - 90, 200, 50 };
    bool hoverPlay = CheckCollisionPointRec(mouse, playRec);
    DrawRectangleRounded(playRec, 0.2f, 5, hoverPlay ? LIGHTGRAY : GRAY);
    DrawText("PLAY", cx - MeasureText("PLAY", 20)/2, cy - 75, 20, WHITE);
    
    if (hoverPlay && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        networkClient->sendPacket(Packet{PacketType::CLIENT_PLAY_REQUEST, {}});
        currentState = UIState::WAITING; // Safely lock the screen onto matchmaking while the Server drops the block!
    }

    // Leaderboard Button
    Rectangle ldRec = { cx - 100, cy - 10, 200, 50 };
    bool hoverLd = CheckCollisionPointRec(mouse, ldRec);
    DrawRectangleRounded(ldRec, 0.2f, 5, hoverLd ? LIGHTGRAY : GRAY);
    DrawText("LEADERBOARD", cx - MeasureText("LEADERBOARD", 20)/2, cy + 5, 20, WHITE);
    
    if (hoverLd && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentState = UIState::LEADERBOARD;
    }

    // Exit Button
    Rectangle exRec = { cx - 100, cy + 70, 200, 50 };
    bool hoverEx = CheckCollisionPointRec(mouse, exRec);
    DrawRectangleRounded(exRec, 0.2f, 5, hoverEx ? MAROON : RED);
    DrawText("EXIT", cx - MeasureText("EXIT", 20)/2, cy + 85, 20, WHITE);
    
    if (hoverEx && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        exitTriggered = true; 
    }
}

void UIManager::drawLeaderboardScreen() {
    float cx = logicalWidth / 2.0f;
    float cy = logicalHeight / 2.0f;
    
    drawPanel(cx - 250, cy - 200, 500, 400, "GLOBAL LEADERBOARD", 25);
    
    float startY = cy - 110;
    for (size_t i = 0; i < leaderboard.size(); ++i) {
        std::string rankStr = "#" + std::to_string(i + 1) + "  " + leaderboard[i].first;
        std::string winStr = std::to_string(leaderboard[i].second) + " Wins";
        
        DrawText(rankStr.c_str(), cx - 200, startY, 20, DARKGRAY);
        DrawText(winStr.c_str(), cx + 100, startY, 20, DARKGREEN);
        startY += 40;
    }

    Rectangle backRec = { cx - 100, cy + 110, 200, 50 };
    Vector2 mouse = getLogicalMousePos();
    bool hoverBack = CheckCollisionPointRec(mouse, backRec);
    DrawRectangleRounded(backRec, 0.2f, 5, hoverBack ? LIGHTGRAY : GRAY);
    DrawText("BACK", cx - MeasureText("BACK", 20)/2, cy + 125, 20, WHITE);

    if (hoverBack && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentState = UIState::MAIN_MENU;
    }
}

void UIManager::drawWaitingScreen() {
    float cx = logicalWidth / 2.0f;
    float cy = logicalHeight / 2.0f;
    
    drawPanel(cx - 250, cy - 100, 500, 200, "MATCHMAKING", 25);
    DrawText("Searching for a worthy opponent...", cx - MeasureText("Searching for a worthy opponent...", 20)/2, cy - 10, 20, MAROON);
}

void UIManager::drawGameOverScreen() {
    float cx = logicalWidth / 2.0f;
    float cy = logicalHeight / 2.0f;
    
    drawPanel(cx - 200, cy - 150, 400, 300, "MATCH CONCLUDED", 25);
    
    Color msgColor = DARKGRAY;
    if (gameOverMessage == "VICTORY") msgColor = GOLD;
    else if (gameOverMessage == "DEFEAT") msgColor = MAROON;
    
    DrawText(gameOverMessage.c_str(), cx - MeasureText(gameOverMessage.c_str(), 45)/2, cy - 60, 45, msgColor);
    
    // Main Menu Return Button Array
    Rectangle menuRec = { cx - 180, cy + 50, 160, 50 };
    Vector2 mouse = getLogicalMousePos();
    bool hoverMenu = CheckCollisionPointRec(mouse, menuRec);
    DrawRectangleRounded(menuRec, 0.2f, 5, hoverMenu ? LIGHTGRAY : GRAY);
    DrawText("MAIN MENU", cx - 180 + 80 - MeasureText("MAIN MENU", 15)/2, cy + 68, 15, WHITE);

    if (hoverMenu && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentState = UIState::MAIN_MENU;
    }

    // Rematch Execution Button Array
    Rectangle playRec = { cx + 20, cy + 50, 160, 50 };
    bool hoverPlay = CheckCollisionPointRec(mouse, playRec);
    DrawRectangleRounded(playRec, 0.2f, 5, hoverPlay ? LIME : GREEN);
    DrawText("PLAY AGAIN", cx + 20 + 80 - MeasureText("PLAY AGAIN", 15)/2, cy + 68, 15, WHITE);

    if (hoverPlay && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        networkClient->sendPacket(Packet{PacketType::CLIENT_PLAY_REQUEST, {}});
        currentState = UIState::WAITING;
        combatLog.clear(); // Safely clean up the UI components seamlessly for the next fresh match sequence!
        combatLog.push_back("SYSTEM: Awaiting Rematch initialization...");
    }
}

void UIManager::updateAndDraw() {
    processNetworkPackets(); 
    
    if (currentState == UIState::LOGIN) {
        handleLoginInput();
    } else if (currentState == UIState::BATTLE) {
        handleKeyboardInput();
    }

    BeginTextureMode(target);
    ClearBackground(Color{15, 35, 45, 255});

    if (currentState == UIState::LOGIN) {
        drawLoginScreen();
    } else if (currentState == UIState::MAIN_MENU) {
        drawMainMenu();
    } else if (currentState == UIState::LEADERBOARD) {
        drawLeaderboardScreen();
    } else if (currentState == UIState::WAITING) {
        drawWaitingScreen();
    } else if (currentState == UIState::BATTLE) {
        drawTopPanel();
        drawWordPool();
        drawCombatLog();
        drawSentenceBuilder();
    } else if (currentState == UIState::GAMEOVER) {
        drawGameOverScreen();
    }

    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK); 

    float scale = std::min((float)GetScreenWidth() / logicalWidth, (float)GetScreenHeight() / logicalHeight);
    Rectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height };
    Rectangle destRec = { 
        (GetScreenWidth() - (logicalWidth * scale)) * 0.5f, 
        (GetScreenHeight() - (logicalHeight * scale)) * 0.5f, 
        logicalWidth * scale, logicalHeight * scale 
    };
    
    DrawTexturePro(target.texture, sourceRec, destRec, Vector2{ 0, 0 }, 0.0f, WHITE);
    EndDrawing();
}

void UIManager::drawTopPanel() {
    float padding = 30.0f;
    float topY = 30.0f;
    float panelHeight = 100.0f;

    // Player Status
    Rectangle p1Rec = { padding, topY, 300, panelHeight };
    DrawRectangleRounded(p1Rec, 0.15f, 10, Color{230, 230, 230, 255});
    std::string p1Title = authUsername.empty() ? "Player" : authUsername;
    DrawText(p1Title.c_str(), p1Rec.x + 15, p1Rec.y + 15, 20, DARKGRAY);
    
    DrawText(TextFormat("HP: %i / %i", playerHp, playerMaxHp), p1Rec.x + 15, p1Rec.y + 40, 15, DARKGRAY);
    Rectangle hpBg = { p1Rec.x + 150, p1Rec.y + 40, 130, 15 };
    DrawRectangleRounded(hpBg, 0.5f, 5, DARKGRAY);
    float p1HpRatio = playerMaxHp > 0 ? (float)playerHp / (float)playerMaxHp : 0;
    Rectangle hpFg = { p1Rec.x + 150, p1Rec.y + 40, 130.0f * p1HpRatio, 15 }; 
    DrawRectangleRounded(hpFg, 0.5f, 5, MAROON);

    DrawText(TextFormat("Shield: %i / %i", playerShield, playerMaxHp), p1Rec.x + 15, p1Rec.y + 65, 15, DARKGRAY);
    Rectangle shBg = { p1Rec.x + 150, p1Rec.y + 65, 130, 15 };
    DrawRectangleRounded(shBg, 0.5f, 5, DARKGRAY);
    float p1ShRatio = playerMaxHp > 0 ? (float)playerShield / (float)playerMaxHp : 0;
    Rectangle shFg = { p1Rec.x + 150, p1Rec.y + 65, std::min(130.0f, 130.0f * p1ShRatio), 15 }; 
    DrawRectangleRounded(shFg, 0.5f, 5, SKYBLUE);

    // Opponent Status
    Rectangle p2Rec = { logicalWidth - padding - 300, topY, 300, panelHeight };
    DrawRectangleRounded(p2Rec, 0.15f, 10, Color{230, 230, 230, 255});
    DrawText(opponentName.c_str(), p2Rec.x + 15, p2Rec.y + 15, 20, DARKGRAY);
    
    DrawText(TextFormat("HP: %i / %i", opponentHp, opponentMaxHp), p2Rec.x + 15, p2Rec.y + 40, 15, DARKGRAY);
    Rectangle hpBg2 = { p2Rec.x + 150, p2Rec.y + 40, 130, 15 };
    DrawRectangleRounded(hpBg2, 0.5f, 5, DARKGRAY);
    float p2HpRatio = opponentMaxHp > 0 ? (float)opponentHp / (float)opponentMaxHp : 0;
    Rectangle hpFg2 = { p2Rec.x + 150, p2Rec.y + 40, 130.0f * p2HpRatio, 15 }; 
    DrawRectangleRounded(hpFg2, 0.5f, 5, MAROON);

    DrawText(TextFormat("Shield: %i / %i", opponentShield, opponentMaxHp), p2Rec.x + 15, p2Rec.y + 65, 15, DARKGRAY);
    Rectangle shBg2 = { p2Rec.x + 150, p2Rec.y + 65, 130, 15 };
    DrawRectangleRounded(shBg2, 0.5f, 5, DARKGRAY);
    float p2ShRatio = opponentMaxHp > 0 ? (float)opponentShield / (float)opponentMaxHp : 0;
    Rectangle shFg2 = { p2Rec.x + 150, p2Rec.y + 65, std::min(130.0f, 130.0f * p2ShRatio), 15 }; 
    DrawRectangleRounded(shFg2, 0.5f, 5, SKYBLUE);
}

void UIManager::drawWordPool() {
    float padding = 30.0f;
    float yOffset = 140.0f;
    float wpWidth = logicalWidth * 0.45f;
    float wpHeight = 350.0f;
    drawPanel(padding, yOffset, wpWidth, wpHeight, "Word Pool (Type words from here)", 20);

    // Group words into categorical columns for visual UX efficiency
    std::vector<Word> colSubjects, colActions, colModifiers, colConnectors;
    for (const auto& w : currentPool) {
        switch(w.getPartOfSpeech()) {
            case PartOfSpeech::Noun:
            case PartOfSpeech::Pronoun:
                colSubjects.push_back(w); break;
            case PartOfSpeech::Verb:
                colActions.push_back(w); break;
            case PartOfSpeech::Adjective:
            case PartOfSpeech::Adverb:
                colModifiers.push_back(w); break;
            default:
                colConnectors.push_back(w); break;
        }
    }

    auto drawColumn = [&](const std::vector<Word>& col, float startX, const char* title) {
        DrawText(title, startX, yOffset + 35, 15, DARKGRAY);
        float currentY = yOffset + 60;
        for (const auto& w : col) {
            std::string label = w.getText();
            int nameWidth = MeasureText(label.c_str(), 15);
            
            std::string valStr = std::to_string(w.getEffect().getValue());
            int badgeWidth = MeasureText(valStr.c_str(), 12) + 8;
            
            int btnWidth = nameWidth + badgeWidth + 20;

            Rectangle btnRec = { startX, currentY, (float)btnWidth, 25 };
            Color boxColor = getEffectColor(w.getEffect().getType());
            DrawRectangleRounded(btnRec, 0.2f, 5, boxColor);
            
            DrawText(label.c_str(), startX + 8, currentY + 5, 15, BLACK);
            
            Rectangle badgeRec = { startX + btnWidth - badgeWidth - 4, currentY + 4, (float)badgeWidth, 17 };
            DrawRectangleRounded(badgeRec, 0.5f, 5, Color{0, 0, 0, 60});
            DrawText(valStr.c_str(), badgeRec.x + 4, badgeRec.y + 2, 12, WHITE);

            currentY += 32;
        }
    };

    float cWidth = (wpWidth - 30) / 4.0f;
    drawColumn(colSubjects, padding + 15, "Subjects");
    drawColumn(colActions, padding + 15 + cWidth, "Actions");
    drawColumn(colModifiers, padding + 15 + cWidth * 2, "Modifiers");
    drawColumn(colConnectors, padding + 15 + cWidth * 3, "Connectors");
}

static void drawRichText(const std::string& text, float startX, float startY, int fontSize, const std::string& authUsername, const std::string& opponentName) {
    Color baseColor = DARKGRAY;
    
    // Globally isolate Base Color using Player string validations!
    if (text.find("SYSTEM") == 0 || text.find("Turn") == 0 || text.find("->") == 0 || text.find("Valid") == 0 || text.find("Invalid") == 0) {
        baseColor = GRAY;
    } else if (!authUsername.empty() && (text.find(authUsername) == 0 || text.find("Opponent mumbled") != std::string::npos)) {
        baseColor = Color{100, 150, 255, 255}; // Player blue
    } else if (!opponentName.empty() && (text.find(opponentName) == 0 || text.find("Opponent attacked") != std::string::npos)) {
        baseColor = Color{255, 100, 100, 255}; // Enemy ref
    }

    float currentX = startX;
    std::stringstream ss(text);
    std::string word;
    
    // Mathematically tokenize the line string by blank spaces, projecting independent RGB bounds over targeted tokens sequentially
    while (ss >> word) {
        Color c = baseColor;
        if (word.find("damage") != std::string::npos) c = Color{240, 90, 90, 255};
        else if (word.find("healed") != std::string::npos || word.find("HP") != std::string::npos) c = Color{110, 220, 110, 255};
        else if (word.find("shield") != std::string::npos) c = Color{90, 180, 240, 255};
        else if (word.find("weakened") != std::string::npos) c = Color{180, 110, 220, 255};
        else if (word.find("CRITICAL") != std::string::npos) c = Color{240, 160, 60, 255};
        else if (word.find("COMBO!") != std::string::npos) c = GOLD;

        // Render distinct token string mapped physically along dynamic cursor X
        DrawText(word.c_str(), currentX, startY, fontSize, c);
        
        // Progress X cursor via exact Token dimensional length combined with logical gap Width natively calculated via the physics vector bounds
        currentX += MeasureText(word.c_str(), fontSize) + MeasureText(" ", fontSize);
    }
}

void UIManager::drawCombatLog() {
    float padding = 30.0f;
    float yOffset = 140.0f;
    float clWidth = logicalWidth * 0.45f;
    float clHeight = 520.0f;
    drawPanel(logicalWidth - padding - clWidth, yOffset, clWidth, clHeight, "Combat Log", 25);

    float ly = yOffset + 50;
    for (size_t i = std::max(0, (int)combatLog.size() - 22); i < combatLog.size(); ++i) {
        drawRichText(combatLog[i], logicalWidth - padding - clWidth + 15, ly, 15, authUsername, opponentName);
        ly += 20;
    }
}

void UIManager::drawSentenceBuilder() {
    float padding = 30.0f;
    float yOffset = 520.0f;
    float sbWidth = logicalWidth * 0.45f;
    float sbHeight = 140.0f;
    drawPanel(padding, yOffset, sbWidth, sbHeight, "Sentence Builder", 20);

    float cursorX = padding + 15;
    
    for (const Word& w : currentSentence.getWords()) {
        std::string label = w.getText();
        std::string valStr = std::to_string(w.getEffect().getValue());
        
        int nameWidth = MeasureText(label.c_str(), 20);
        int badgeWidth = MeasureText(valStr.c_str(), 15) + 10;
        int btnWidth = nameWidth + badgeWidth + 20;
        
        Rectangle btnRec = { cursorX, yOffset + 40, (float)btnWidth, 35 };
        Color boxColor = getEffectColor(w.getEffect().getType());
        DrawRectangleRounded(btnRec, 0.2f, 5, boxColor);
        DrawText(label.c_str(), cursorX + 10, yOffset + 48, 20, BLACK);
        
        Rectangle badgeRec = { cursorX + btnWidth - badgeWidth - 5, yOffset + 45, (float)badgeWidth, 25 };
        DrawRectangleRounded(badgeRec, 0.5f, 5, Color{0, 0, 0, 60});
        DrawText(valStr.c_str(), badgeRec.x + 5, badgeRec.y + 5, 15, WHITE);
        
        cursorX += btnWidth + 5;
    }

    if (!currentSentence.getWords().empty()) cursorX += 10;

    DrawText(typingBuffer.c_str(), cursorX, yOffset + 48, 20, MAROON);
    
    if ((int)(GetTime() * 2) % 2 == 0) {
        DrawText("_", cursorX + MeasureText(typingBuffer.c_str(), 20), yOffset + 48, 20, MAROON);
    }

    Rectangle submitRec = { padding + sbWidth - 165, yOffset + sbHeight - 45, 150, 30 };
    Vector2 mouse = getLogicalMousePos();
    bool hovered = CheckCollisionPointRec(mouse, submitRec) && !waitingForTurn;
    
    DrawRectangleRounded(submitRec, 0.2f, 5, waitingForTurn ? DARKGRAY : (hovered ? LIME : GREEN));
    DrawText("SUBMIT [ENTER]", submitRec.x + 10, submitRec.y + 7, 15, waitingForTurn ? GRAY : WHITE);

    if (hovered && !waitingForTurn && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (!currentSentence.getWords().empty()) {
            nlohmann::json payload = nlohmann::json::array();
            for(const auto& w : currentSentence.getWords()) {
                payload.push_back(w.toJson());
            }
            networkClient->sendPacket(Packet{PacketType::CLIENT_SUBMIT_TURN, payload});
            combatLog.push_back("-> Turn submitted! Waiting for server...");
            currentSentence.clear();
            typingBuffer.clear();
            waitingForTurn = true; // Instantly lock keyboard
        }
    }
}