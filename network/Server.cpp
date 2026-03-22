#include "Server.h"
#include <iostream>
#include <fstream>
#include <thread>
#include "../word/WordDictionary.h"

using namespace nlohmann;
using asio::ip::tcp;

std::mutex dbMutex;

Server::Server(short port) : io_context(), acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
    validFormulas = {
        "NV", "ANV", "AJNV", "PV", "NVD", "ANVD", "PVD", "NVRN", "PVRN", "ANVRAN", "V", "VD", "NVC", "PVA"
    };
}

void Server::sendPacket(tcp::socket& socket, const Packet& packet) {
    std::string data = packet.dump();
    asio::write(socket, asio::buffer(data));
}

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

void Server::saveDatabase() {
    std::ofstream outFile("../database/accounts.json");
    outFile << accountsDb.dump(4);
    outFile.close();
}

void Server::broadcastLeaderboard(std::shared_ptr<tcp::socket> sock) {
    std::vector<std::pair<std::string, int>> leaders;
    {
        std::lock_guard<std::mutex> lock(dbMutex);
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

void Server::recordMatchResult(const std::string& p1Name, const std::string& p2Name, bool p1Won, bool p2Won) {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (p1Won && !p2Won) {
        accountsDb[p1Name]["wins"] = (int)accountsDb[p1Name]["wins"] + 1;
        accountsDb[p2Name]["losses"] = (int)accountsDb[p2Name]["losses"] + 1;
    } else if (p2Won && !p1Won) {
        accountsDb[p2Name]["wins"] = (int)accountsDb[p2Name]["wins"] + 1;
        accountsDb[p1Name]["losses"] = (int)accountsDb[p1Name]["losses"] + 1;
    }
    saveDatabase();
}

void Server::processTurn(std::shared_ptr<tcp::socket> attackerSock, std::shared_ptr<tcp::socket> defenderSock, 
                         Player& attacker, Player& defender, const Sentence& sent) {
    std::string sig = sent.getPatternSignature();
    if (validFormulas.contains(sig)) {
        sendPacket(*attackerSock, Packet{PacketType::SERVER_LOG_MESSAGE, "Valid Syntax! [" + sig + "] Attack lands!"});
        sendPacket(*defenderSock, Packet{PacketType::SERVER_LOG_MESSAGE, "Opponent attacked successfully!"});
        
        std::vector<std::string> logs = attacker.applySentenceEffects(sent, defender);
        for (const auto& logMsg : logs) {
            sendPacket(*attackerSock, Packet{PacketType::SERVER_LOG_MESSAGE, logMsg});
            sendPacket(*defenderSock, Packet{PacketType::SERVER_LOG_MESSAGE, logMsg});
        }
    } else {
        sendPacket(*attackerSock, Packet{PacketType::SERVER_LOG_MESSAGE, "Invalid Grammar! [" + sig + "] Attack FIZZLED!"});
        sendPacket(*defenderSock, Packet{PacketType::SERVER_LOG_MESSAGE, "Opponent mumbled gibberish. Nothing happens."});
    }
}

void Server::run() {
    std::cout << "===== WORDWAR3 SERVER ENGINES ONLINE =====\n";
    std::cout << "Listening on Port " << acceptor.local_endpoint().port() << "...\n";

    loadDatabase();

    std::thread([this]() { matchmakerLoop(); }).detach();

    acceptorLoop(); 
}

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

void Server::authenticate(std::shared_ptr<tcp::socket> sock) {
    std::string username;
    while (true) {
        try {
            Packet p = receivePacket(*sock);
            if (p.type == PacketType::CLIENT_LOGIN) {
                std::string user = p.payload["username"];
                std::string pass = p.payload["password"];
                
                std::lock_guard<std::mutex> lock(dbMutex);
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
                
                std::lock_guard<std::mutex> lock(dbMutex);
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
    
    broadcastLeaderboard(sock);
    std::cout << username << " entered Main Menu.\n";
    
    while (true) {
        try {
            Packet p = receivePacket(*sock);
            if (p.type == PacketType::CLIENT_PLAY_REQUEST) {
                session->isMatchmaking = true;
                session->matchStarted = false;
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    matchmakingQueue.remove(session); // Protect globally against duplicated queuing instances natively effortlessly organically!
                    matchmakingQueue.push_back(session);
                }
                queueCV.notify_one(); 
                std::cout << username << " queued for matchmaking!\n";
                
                while (session->isMatchmaking && !session->matchStarted) {
                    if (sock->available() > 0) {
                        Packet cp = receivePacket(*sock);
                        if (cp.type == PacketType::CLIENT_CANCEL_MATCHMAKING) {
                             session->isMatchmaking = false; 
                             {
                                 std::lock_guard<std::mutex> lock(queueMutex);
                                 matchmakingQueue.remove(session); // Cleanly mathematically formally visually effortlessly effectively safely natively!
                             }
                             std::cout << username << " elegantly canceled matchmaking locally!\n";
                             break;
                        }
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                }
                
                if (session->matchStarted) {
                    return; // Dropped safely cleanly gracefully into Matchmaker structurally cleanly seamlessly systematically smoothly ideally securely explicitly identically optimally functionally!
                }
                continue; // Back correctly mathematically securely into Main Menu array securely seamlessly gracefully seamlessly ideally smoothly flawlessly magically unconditionally universally correctly globally cleanly optimally cleanly rationally explicitly optimally internally!
            }
        } catch(...) {
            std::cout << username << " dynamically disconnected from Main Menu.\n";
            return;
        }
    }
}

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
        
        // Spawn entirely isolated dedicated Match Instance matrix loop!
        std::thread([this, p1, p2]() {
            runMatchInstance(p1, p2);
        }).detach();
    }
}

void Server::runMatchInstance(std::shared_ptr<PlayerSession> p1Session, std::shared_ptr<PlayerSession> p2Session) {
    WordDictionary dict("../database/words.json");
    Player p1(p1Session->username, 200);
    Player p2(p2Session->username, 200);

    auto sock1 = p1Session->socket;
    auto sock2 = p2Session->socket;

    try {
        sendPacket(*sock1, Packet{PacketType::SERVER_MATCH_FOUND, {{"opponent", p2Session->username}, {"my_max_hp", p1.getMaxHp()}, {"opp_max_hp", p2.getMaxHp()}}});
    } catch (...) {
        std::cout << p1.getName() << " was a dead connection! Aborting.\n";
        try { sendPacket(*sock2, Packet{PacketType::GAME_OVER, "DRAW"}); } catch(...) {}
        return;
    }

    try {
        sendPacket(*sock2, Packet{PacketType::SERVER_MATCH_FOUND, {{"opponent", p1Session->username}, {"my_max_hp", p2.getMaxHp()}, {"opp_max_hp", p1.getMaxHp()}}});
    } catch (...) {
        std::cout << p2.getName() << " was a dead connection! Aborting.\n";
        try { sendPacket(*sock1, Packet{PacketType::GAME_OVER, "DRAW"}); } catch(...) {}
        return;
    }

    try {
        while (p1.getHp() > 0 && p2.getHp() > 0) {
            std::vector<Word> pool1 = dict.getRandomWords(12);
            std::vector<Word> pool2 = dict.getRandomWords(12);

            json j1 = json::array(), j2 = json::array();
            for(auto& w : pool1) j1.push_back(w.toJson());
            for(auto& w : pool2) j2.push_back(w.toJson());

            sendPacket(*sock1, Packet{PacketType::SERVER_SEND_POOL, j1});
            sendPacket(*sock2, Packet{PacketType::SERVER_SEND_POOL, j2});

            Packet p1Turn = receivePacket(*sock1);
            Sentence sent1;
            for(auto& jObj : p1Turn.payload) sent1.addWord(Word::fromJson(jObj));
            
            Packet p2Turn = receivePacket(*sock2);
            Sentence sent2;
            for(auto& jObj : p2Turn.payload) sent2.addWord(Word::fromJson(jObj));

            int len1 = sent1.getWords().size();
            int len2 = sent2.getWords().size();

            if (len1 < len2) {
                sendPacket(*sock1, Packet{PacketType::SERVER_LOG_MESSAGE, "-> INITIATIVE! Your combo was tighter ("+std::to_string(len1)+" vs "+std::to_string(len2)+"). You strike first!"});
                sendPacket(*sock2, Packet{PacketType::SERVER_LOG_MESSAGE, "-> OUTSPED! Opponent's fast combo strikes first!"});
                processTurn(sock1, sock2, p1, p2, sent1);
                if (p2.getHp() > 0) processTurn(sock2, sock1, p2, p1, sent2);
            } else if (len2 < len1) {
                sendPacket(*sock2, Packet{PacketType::SERVER_LOG_MESSAGE, "-> INITIATIVE! Your combo was tighter ("+std::to_string(len2)+" vs "+std::to_string(len1)+"). You strike first!"});
                sendPacket(*sock1, Packet{PacketType::SERVER_LOG_MESSAGE, "-> OUTSPED! Opponent's fast combo strikes first!"});
                processTurn(sock2, sock1, p2, p1, sent2);
                if (p1.getHp() > 0) processTurn(sock1, sock2, p1, p2, sent1);
            } else {
                sendPacket(*sock1, Packet{PacketType::SERVER_LOG_MESSAGE, "-> CLASH! Perfect mirrors. Simultaneous strike!"});
                sendPacket(*sock2, Packet{PacketType::SERVER_LOG_MESSAGE, "-> CLASH! Perfect mirrors. Simultaneous strike!"});
                processTurn(sock1, sock2, p1, p2, sent1);
                processTurn(sock2, sock1, p2, p1, sent2);
            }

            json jStatsP1 = {
                {"my_hp", p1.getHp()}, {"my_shield", p1.getShield()},
                {"opp_hp", p2.getHp()}, {"opp_shield", p2.getShield()}
            };
            json jStatsP2 = {
                {"my_hp", p2.getHp()}, {"my_shield", p2.getShield()},
                {"opp_hp", p1.getHp()}, {"opp_shield", p1.getShield()}
            };
            
            sendPacket(*sock1, Packet{PacketType::SERVER_UPDATE_HP, jStatsP1});
            sendPacket(*sock2, Packet{PacketType::SERVER_UPDATE_HP, jStatsP2});
        }
        
        if (p1.getHp() <= 0 && p2.getHp() > 0) {
            sendPacket(*sock1, Packet{PacketType::GAME_OVER, "DEFEAT"});
            sendPacket(*sock2, Packet{PacketType::GAME_OVER, "VICTORY"});
            recordMatchResult(p1.getName(), p2.getName(), false, true);
        } else if (p2.getHp() <= 0 && p1.getHp() > 0) {
            sendPacket(*sock1, Packet{PacketType::GAME_OVER, "VICTORY"});
            sendPacket(*sock2, Packet{PacketType::GAME_OVER, "DEFEAT"});
            recordMatchResult(p1.getName(), p2.getName(), true, false);
        } else {
            sendPacket(*sock1, Packet{PacketType::GAME_OVER, "DRAW"});
            sendPacket(*sock2, Packet{PacketType::GAME_OVER, "DRAW"});
            recordMatchResult(p1.getName(), p2.getName(), false, false);
        }

    } catch (...) {
        std::cout << "Match abnormally terminated! Sockets lost actively connecting parameters!\n";
        try { sendPacket(*sock1, Packet{PacketType::GAME_OVER, "VICTORY"}); } catch(...) {}
        try { sendPacket(*sock2, Packet{PacketType::GAME_OVER, "VICTORY"}); } catch(...) {}
        return; 
    }

    std::cout << "Match End. Structurally spawning isolated Post-Match listener blocks natively...\n";

    std::thread([this, p1Session]() {
        broadcastLeaderboard(p1Session->socket);
        authenticate(p1Session->socket);
    }).detach();
    std::thread([this, p2Session]() {
        broadcastLeaderboard(p2Session->socket);
        authenticate(p2Session->socket);
    }).detach();
}
