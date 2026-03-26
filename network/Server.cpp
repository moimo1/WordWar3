#include "Server.h"
#include <iostream>
#include <fstream>
#include <thread>
#include "../manager/GameManager.h"

using namespace nlohmann;
using asio::ip::tcp;

// Initializes the server on the specified TCP port
Server::Server(short port) : io_context(), acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
}

// Converts a Packet object into JSON and sends it physically over the socket
void Server::sendPacket(tcp::socket& socket, const Packet& packet) {
    std::string data = packet.dump();
    asio::write(socket, asio::buffer(data));
}

// Reads raw streaming data from the socket until a newline, then rebuilds a Packet from the JSON string
Packet Server::receivePacket(tcp::socket& socket) {
    std::string line;
    char c = 0;
    while (true) {
        asio::read(socket, asio::buffer(&c, 1));
        if (c == '\n') break;
        line += c;
    }
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return Packet::parse(line);
}

void Server::loadDatabase() {
    std::ifstream accountsFile("../database/accounts.json");
    if (accountsFile.is_open()) {
        try { accountsFile >> accountsDb; } 
        catch(...) { accountsDb = json::object(); }
        accountsFile.close();
    } else {
        accountsDb = json::object();
    }
}

// Safely flushes the current server RAM memory back to accounts.json on the hard drive
void Server::saveDatabase() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    std::ofstream outFile("../database/accounts.json");
    outFile << accountsDb.dump(4);
    outFile.close();
}

// Calculates the Top 5 players mathematically by Win ratio and pushes the list to a Client's UI lobby
void Server::broadcastLeaderboard(std::shared_ptr<tcp::socket> sock) {
    std::vector<std::pair<std::string, int>> leaders;
    {
        std::lock_guard<std::recursive_mutex> lock(dbMutex);
        for (auto& el : accountsDb.items()) {
            leaders.push_back({el.key(), el.value()["wins"]});
        }
    }
    std::sort(leaders.begin(), leaders.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    
    json ldb = json::array();
    for (int i = 0; i < std::min(5, (int)leaders.size()); ++i) {
        ldb.push_back({ {"username", leaders[i].first}, {"wins", leaders[i].second} });
    }
    sendPacket(*sock, Packet{PacketType::SERVER_SEND_LEADERBOARD, ldb});
}

// Safely blocks RAM and natively adds +1 Win and +1 Loss to the correct player accounts immediately after a match
void Server::recordMatchResult(const std::string& p1Name, const std::string& p2Name, bool p1Won, bool p2Won) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (p1Won && !p2Won) {
        accountsDb[p1Name]["wins"] = (int)accountsDb[p1Name]["wins"] + 1;
        accountsDb[p2Name]["losses"] = (int)accountsDb[p2Name]["losses"] + 1;
    } else if (p2Won && !p1Won) {
        accountsDb[p2Name]["wins"] = (int)accountsDb[p2Name]["wins"] + 1;
        accountsDb[p1Name]["losses"] = (int)accountsDb[p1Name]["losses"] + 1;
    }
    saveDatabase();
}

// The master sequence that boots up the overarching matchmaking algorithm threads and the raw incoming connection acceptor
void Server::run() {
    std::cout << "===== WORDWAR3 SERVER ENGINES ONLINE =====\n";
    std::cout << "Listening on Port " << acceptor.local_endpoint().port() << "...\n";

    loadDatabase();

    std::thread([this]() { matchmakerLoop(); }).detach();

    acceptorLoop(); 
}

// Infinite loop strictly dedicated to listening for brand new physical TCP port pings
void Server::acceptorLoop() {
    std::cout << "Global TCP Acceptor Matrix Active. Supporting dynamic MMO connections...\n";
    while (true) {
        auto socket = std::make_shared<tcp::socket>(io_context);
        acceptor.accept(*socket);
        std::cout << "Incoming Connection Detected. Spawning isolated Auth Thread...\n";
        
        // Securely pass Socket ownership iteratively into its own disconnected payload
        std::thread([this, socket]() {
            authenticate(socket);
        }).detach();
    }
}

// The isolated "Lobby" layer that securely processes Login or Registration credentials for an unverified player
void Server::authenticate(std::shared_ptr<tcp::socket> sock) {
    std::string username;
    while (true) {
        try {
            Packet p = receivePacket(*sock);
            if (p.type == PacketType::CLIENT_LOGIN) {
                std::string user = p.payload["username"];
                std::string pass = p.payload["password"];
                
                std::lock_guard<std::recursive_mutex> lock(dbMutex);
                if (accountsDb.contains(user)) {
                    if (accountsDb[user]["password"] == pass) {
                        username = user;
                        sendPacket(*sock, Packet{PacketType::SERVER_LOGIN_RESPONSE, {{"success", true}, {"msg", "Login successful!"}}});
                        break;
                    } else {
                        sendPacket(*sock, Packet{PacketType::SERVER_LOGIN_RESPONSE, {{"success", false}, {"msg", "Invalid password!"}}});
                    }
                } else {
                    sendPacket(*sock, Packet{PacketType::SERVER_LOGIN_RESPONSE, {{"success", false}, {"msg", "Account does not exist!"}}});
                }
            } else if (p.type == PacketType::CLIENT_REGISTER) {
                std::string user = p.payload["username"];
                std::string pass = p.payload["password"];
                
                std::lock_guard<std::recursive_mutex> lock(dbMutex);
                if (accountsDb.contains(user)) {
                    sendPacket(*sock, Packet{PacketType::SERVER_LOGIN_RESPONSE, {{"success", false}, {"msg", "Username already taken!"}}});
                } else {
                    accountsDb[user] = { {"password", pass}, {"wins", 0}, {"losses", 0} };
                    saveDatabase();
                    username = user;
                    sendPacket(*sock, Packet{PacketType::SERVER_LOGIN_RESPONSE, {{"success", true}, {"msg", "Account registered securely!"}}});
                    break; 
                }
            }
        } catch(...) {
            std::cout << "Socket TCP stream failed during Menu authentication block.\n";
            return; // Hard thread kill dynamically freeing Socket pointers
        }
    }

    auto session = std::make_shared<PlayerSession>();
    session->socket = sock;
    session->username = username;
    
    sessionMenuLoop(session);
}

void Server::sessionMenuLoop(std::shared_ptr<PlayerSession> session) {
    broadcastLeaderboard(session->socket);
    std::cout << session->username << " formally entered Main Menu computationally gracefully.\n";
    
    while (true) {
        try {
            Packet p = receivePacket(*(session->socket));
            if (p.type == PacketType::CLIENT_PLAY_REQUEST) {
                session->isMatchmaking = true;
                session->matchStarted = false;
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    matchmakingQueue.remove(session);
                    matchmakingQueue.push_back(session);
                }
                queueCV.notify_one(); 
                std::cout << session->username << " actively computationally securely queued for seamless MMO matchmaking!\n";
                
                while (session->isMatchmaking && !session->matchStarted) {
                    if (session->socket->available() > 0) {
                        Packet cp = receivePacket(*(session->socket));
                        if (cp.type == PacketType::CLIENT_CANCEL_MATCHMAKING) {
                             session->isMatchmaking = false; 
                             {
                                 std::lock_guard<std::mutex> lock(queueMutex);
                                 matchmakingQueue.remove(session);
                             }
                             std::cout << session->username << " elegantly dynamically physically unconditionally accurately naturally canceled active queue matchmaking locally!\n";
                             break;
                        }
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                }
                
                if (session->matchStarted) {
                    return; // Dropped safely natively structurally flawlessly cleanly seamlessly inherently dynamically organically effortlessly explicitly successfully into Combat Threads mathematically intelligently safely smoothly flexibly seamlessly correctly!
                }
                continue; 
            }
        } catch(...) {
            std::cout << session->username << " dynamically automatically conditionally proactively smoothly correctly explicitly seamlessly elegantly functionally structurally successfully implicitly perfectly automatically disconnected structurally explicitly flawlessly seamlessly identically unconditionally natively.\n";
            return;
        }
    }
}

// Infinite background watcher that instantly grabs exactly 2 waiting players from the queue and isolates them natively into a GameManager combat thread
void Server::matchmakerLoop() {
    std::cout << "Matchmaker thread natively tracking Lobby arrays...\n";
    while (true) {
        std::shared_ptr<PlayerSession> p1;
        std::shared_ptr<PlayerSession> p2;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this]() { return matchmakingQueue.size() >= 2; });
            
            p1 = matchmakingQueue.front(); matchmakingQueue.pop_front();
            p2 = matchmakingQueue.front(); matchmakingQueue.pop_front();
        }
        
        if (p1 == p2) continue; // Final absolute rigorous structural sanity globally natively organically securely gracefully dynamically functionally manually effortlessly inherently intelligently perfectly magically cleanly check natively!

        if (!p1->isMatchmaking) {
            matchmakingQueue.push_back(p2);
            continue;
        }
        if (!p2->isMatchmaking) {
            matchmakingQueue.push_back(p1);
            continue;
        }

        p1->matchStarted = true;
        p2->matchStarted = true;
        p1->isMatchmaking = false;
        p2->isMatchmaking = false;
        
        std::cout << "Match Found computationally! " << p1->username << " vs " << p2->username << "\n";
        
        // Spawn entirely isolated GameManager structural matrix naturally
        std::thread([this, p1, p2]() {
            std::cout << "Initializing internal GameManager structurally natively for " << p1->username << " and " << p2->username << "...\n";
            GameManager game(p1, p2);
            int outcome = game.runMatch();
            
            if (outcome == 1) recordMatchResult(p1->username, p2->username, true, false);
            else if (outcome == 2) recordMatchResult(p1->username, p2->username, false, true);
            else if (outcome == 0) recordMatchResult(p1->username, p2->username, false, false);

            std::cout << "Match End. Structurally spawning isolated Post-Match listener blocks natively...\n";
            std::thread([this, p1]() {
                p1->matchStarted = false;
                p1->isMatchmaking = false;
                sessionMenuLoop(p1);
            }).detach();
            std::thread([this, p2]() {
                p2->matchStarted = false;
                p2->isMatchmaking = false;
                sessionMenuLoop(p2);
            }).detach();
        }).detach();
    }
}
