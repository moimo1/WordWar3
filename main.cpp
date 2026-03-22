#include <iostream>
#include "manager/UIManager.h"
#include "word/WordDictionary.h"

int main() {
    std::cout << "Loading Word Database from JSON...\n";
    // Path uses ../ because Clion CMake runs from cmake-build-debug
    WordDictionary dict("../database/words.json");
    std::cout << "Successfully loaded " << dict.getDatabaseSize() << " words from the file!\n";

    std::cout << "Starting WordWar3 UI Window...\n";
    
    // Launch Raylib GUI manager using 1280x720 window
    UIManager ui(1280, 720, "WordWar3 - UI Layout Prototype");

    // Main game loop
    while (!ui.shouldClose()) {
        ui.updateAndDraw();
    }

    return 0;
}