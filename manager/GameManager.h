#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <memory>
#include <set>
#include <string>
#include "../network/Server.h"
#include "../player/Player.h"
#include "../player/Sentence.h"

class GameManager {
private:
    std::shared_ptr<PlayerSession> p1Session;
    std::shared_ptr<PlayerSession> p2Session;
    std::set<std::string> validFormulas;

    void processTurn(std::shared_ptr<asio::ip::tcp::socket> attackerSock, std::shared_ptr<asio::ip::tcp::socket> defenderSock, Player& attacker, Player& defender, const Sentence& sent);

public:
    GameManager(std::shared_ptr<PlayerSession> p1, std::shared_ptr<PlayerSession> p2);
    int runMatch();
};

#endif //GAMEMANAGER_H