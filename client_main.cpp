#include <iostream>
#include <thread>
#include <fstream>
#include <string>
#include "manager/UIManager.h"
#include "network/Client.h"

int main(int argc, char* argv[]) {
    std::cout << "Starting WordWar3 CLIENT...\n";
    
    std::string ip = "192.168.24.90";
    std::ifstream cfg("server_ip.txt");
    if (cfg.is_open()) {
        std::getline(cfg, ip);
        cfg.close();
    } else if (argc > 1) {
        ip = argv[1];
    }
    
    std::cout << "Targeting Server IP: " << ip << "\n";

    // Native TCP asynchronous bind
    Client networkClient;
    if (!networkClient.connectToServer(ip, 12345)) {
        std::cerr << "CRITICAL: Could not reach server. Did you start WordWar3_Server.exe first?\n";
        return 1;
    }
    
    // Wire up securely via pointer mapping
    UIManager ui(1280, 720, "WordWar3 - Multiplayer Client", &networkClient);

    while (!ui.shouldClose()) {
        ui.updateAndDraw();
    }

    return 0;
}
