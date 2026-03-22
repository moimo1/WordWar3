#ifndef SERVER_H
#define SERVER_H

#include <asio.hpp>
#include <memory>
#include <string>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <nlohmann/json.hpp>
#include <set>
#include "Packet.h"
#include "../player/Player.h"
#include "../player/Sentence.h"

struct PlayerSession {
    std::shared_ptr<asio::ip::tcp::socket> socket;
    std::string username;
};

class Server {
private:
    asio::io_context io_context;
    asio::ip::tcp::acceptor acceptor;

    nlohmann::json accountsDb;
    std::mutex dbMutex;

    // MMO Matchmaking Infrastructure Arrays
    std::queue<std::shared_ptr<PlayerSession>> matchmakingQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;

    std::set<std::string> validFormulas;

    static void sendPacket(asio::ip::tcp::socket& socket, const Packet& packet);
    static Packet receivePacket(asio::ip::tcp::socket& socket);

    void loadDatabase();
    void saveDatabase();
    
    void broadcastLeaderboard(std::shared_ptr<asio::ip::tcp::socket> sock);
    void recordMatchResult(const std::string& p1Name, const std::string& p2Name, bool p1Won, bool p2Won);
    void processTurn(std::shared_ptr<asio::ip::tcp::socket> attackerSock, std::shared_ptr<asio::ip::tcp::socket> defenderSock, Player& attacker, Player& defender, const Sentence& sent);

    // Advanced Event-Driven Thread Handlers
    void acceptorLoop();
    void matchmakerLoop();
    void authenticate(std::shared_ptr<asio::ip::tcp::socket> sock);
    void runMatchInstance(std::shared_ptr<PlayerSession> p1, std::shared_ptr<PlayerSession> p2);

public:
    Server(short port);
    void run();
};

#endif // SERVER_H
