#include "GameManager.h"
#include <iostream>
#include "../word/WordDictionary.h"
#include <chrono>
#include <thread>

using namespace nlohmann;

// Constructs the active GameManager class and natively builds the entire map of authorized grammar structures
GameManager::GameManager(std::shared_ptr<PlayerSession> p1, std::shared_ptr<PlayerSession> p2) : p1Session(p1), p2Session(p2) {
    validFormulas = {
        // Base
        "V", "VD", "NV", "PV", "NVD", "PVD",
        
        // Simple Transitive
        "NVN", "PVN", "ANV", "APV", "NVP", "PVP",

        // Descriptive Arrays
        "AJNV", "AJJNV", "JNV", "JPV", "ANDV", "PDV", "NDV", "ADV",

        // Deep Transitive & Objects
        "ANVN", "ANVP", "AJNVN", "ANVAN", "NVJ", "NVJN", "PVJ", "PVJN", "ANVJ", "ANVJN",

        // Adverb Extensions
        "ANVD", "VND", "NVND", "ANVND", "PVND", "PVDN", "NVDN",

        // Prepositional Targeting (At, On, Against)
        "NVRN", "PVRN", "ANVRAN", "NVRJN", "PVRJN", "ANVRP", "NVRAN",

        // Compound Constructs
        "NCNV", "PCNV", "NVCN", "PNCV", "ANCNV", "NVNCN", "NVC", "PVA",

        // Massive 6/7-Word Combos
        "AJNVAN", "AJNVRAN", "ANVDRAN", "AJJNVAN", "PVDRAN"
    };
}

// Compares a submitted sentence mathematically against the active formulas to calculate raw magic payload impacts explicitly
void GameManager::processTurn(std::shared_ptr<asio::ip::tcp::socket> attackerSock, std::shared_ptr<asio::ip::tcp::socket> defenderSock, Player& attacker, Player& defender, const Sentence& sent) {
    std::string sig = sent.getPatternSignature();
    std::string fullText = "";
    for (const auto& w : sent.getWords()) fullText += w.getText() + " ";
    if (!fullText.empty()) fullText.pop_back();

    if (validFormulas.contains(sig)) {
        Server::sendPacket(*attackerSock, Packet{PacketType::SERVER_LOG_MESSAGE, "VALID SYNTAX [" + sig + "]: You cast '" + fullText + "'"});
        Server::sendPacket(*defenderSock, Packet{PacketType::SERVER_LOG_MESSAGE, "DANGER: " + attacker.getName() + " cast '" + fullText + "'!"});
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        
        std::vector<std::string> logs = attacker.applySentenceEffects(sent, defender);
        for (const auto& logMsg : logs) {
            Server::sendPacket(*attackerSock, Packet{PacketType::SERVER_LOG_MESSAGE, logMsg});
            Server::sendPacket(*defenderSock, Packet{PacketType::SERVER_LOG_MESSAGE, logMsg});
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
        }
    } else {
        Server::sendPacket(*attackerSock, Packet{PacketType::SERVER_LOG_MESSAGE, "INVALID GRAMMAR [" + sig + "]: Your attack completely FIZZLED!"});
        Server::sendPacket(*defenderSock, Packet{PacketType::SERVER_LOG_MESSAGE, attacker.getName() + " mumbled gibberish! Their attack failed."});
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

// The core, infinitely spinning physical 1v1 Combat Engine dynamically pushing 12-word pools seamlessly and checking player HP bounds
int GameManager::runMatch() {
    WordDictionary dict("../database/words.json");
    Player p1(p1Session->username, 200);
    Player p2(p2Session->username, 200);

    auto sock1 = p1Session->socket;
    auto sock2 = p2Session->socket;

    try {
        Server::sendPacket(*sock1, Packet{PacketType::SERVER_MATCH_FOUND, {{"opponent", p2Session->username}, {"my_max_hp", p1.getMaxHp()}, {"opp_max_hp", p2.getMaxHp()}}});
    } catch (...) {
        std::cout << p1.getName() << " was a dead connection! Aborting.\n";
        try { Server::sendPacket(*sock2, Packet{PacketType::GAME_OVER, "DRAW"}); } catch(...) {}
        return -1;
    }

    try {
        Server::sendPacket(*sock2, Packet{PacketType::SERVER_MATCH_FOUND, {{"opponent", p1Session->username}, {"my_max_hp", p2.getMaxHp()}, {"opp_max_hp", p1.getMaxHp()}}});
    } catch (...) {
        std::cout << p2.getName() << " was a dead connection! Aborting.\n";
        try { Server::sendPacket(*sock1, Packet{PacketType::GAME_OVER, "DRAW"}); } catch(...) {}
        return -1;
    }

    try {
        while (p1.getHp() > 0 && p2.getHp() > 0) {
            std::vector<Word> pool1 = dict.getRandomWords(12);
            std::vector<Word> pool2 = dict.getRandomWords(12);

            json j1 = json::array(), j2 = json::array();
            for(auto& w : pool1) j1.push_back(w.toJson());
            for(auto& w : pool2) j2.push_back(w.toJson());

            Server::sendPacket(*sock1, Packet{PacketType::SERVER_SEND_POOL, j1});
            Server::sendPacket(*sock2, Packet{PacketType::SERVER_SEND_POOL, j2});

            Packet p1Turn = Server::receivePacket(*sock1);
            Sentence sent1;
            for(auto& jObj : p1Turn.payload) sent1.addWord(Word::fromJson(jObj));
            
            Packet p2Turn = Server::receivePacket(*sock2);
            Sentence sent2;
            for(auto& jObj : p2Turn.payload) sent2.addWord(Word::fromJson(jObj));

            int len1 = sent1.getWords().size();
            int len2 = sent2.getWords().size();

            Server::sendPacket(*sock1, Packet{PacketType::SERVER_LOG_MESSAGE, "======================================="});
            Server::sendPacket(*sock2, Packet{PacketType::SERVER_LOG_MESSAGE, "======================================="});
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            if (len1 < len2) {
                Server::sendPacket(*sock1, Packet{PacketType::SERVER_LOG_MESSAGE, "-> INITIATIVE! Your combo was tighter ("+std::to_string(len1)+" vs "+std::to_string(len2)+"). You strike first!"});
                Server::sendPacket(*sock2, Packet{PacketType::SERVER_LOG_MESSAGE, "-> OUTSPED! Opponent's fast combo strikes first!"});
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                processTurn(sock1, sock2, p1, p2, sent1);
                if (p2.getHp() > 0) processTurn(sock2, sock1, p2, p1, sent2);
            } else if (len2 < len1) {
                Server::sendPacket(*sock2, Packet{PacketType::SERVER_LOG_MESSAGE, "-> INITIATIVE! Your combo was tighter ("+std::to_string(len2)+" vs "+std::to_string(len1)+"). You strike first!"});
                Server::sendPacket(*sock1, Packet{PacketType::SERVER_LOG_MESSAGE, "-> OUTSPED! Opponent's fast combo strikes first!"});
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                processTurn(sock2, sock1, p2, p1, sent2);
                if (p1.getHp() > 0) processTurn(sock1, sock2, p1, p2, sent1);
            } else {
                Server::sendPacket(*sock1, Packet{PacketType::SERVER_LOG_MESSAGE, "-> CLASH! Perfect mirrors. Simultaneous strike!"});
                Server::sendPacket(*sock2, Packet{PacketType::SERVER_LOG_MESSAGE, "-> CLASH! Perfect mirrors. Simultaneous strike!"});
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
            
            Server::sendPacket(*sock1, Packet{PacketType::SERVER_UPDATE_HP, jStatsP1});
            Server::sendPacket(*sock2, Packet{PacketType::SERVER_UPDATE_HP, jStatsP2});
        }
        
        if (p1.getHp() <= 0 && p2.getHp() > 0) {
            Server::sendPacket(*sock1, Packet{PacketType::GAME_OVER, "DEFEAT"});
            Server::sendPacket(*sock2, Packet{PacketType::GAME_OVER, "VICTORY"});
            return 2;
        } else if (p2.getHp() <= 0 && p1.getHp() > 0) {
            Server::sendPacket(*sock1, Packet{PacketType::GAME_OVER, "VICTORY"});
            Server::sendPacket(*sock2, Packet{PacketType::GAME_OVER, "DEFEAT"});
            return 1;
        } else {
            Server::sendPacket(*sock1, Packet{PacketType::GAME_OVER, "DRAW"});
            Server::sendPacket(*sock2, Packet{PacketType::GAME_OVER, "DRAW"});
            return 0;
        }

    } catch (...) {
        std::cout << "Match abnormally terminated! Sockets lost actively connecting parameters!\n";
        try { Server::sendPacket(*sock1, Packet{PacketType::GAME_OVER, "VICTORY"}); } catch(...) {}
        try { Server::sendPacket(*sock2, Packet{PacketType::GAME_OVER, "VICTORY"}); } catch(...) {}
        return -1; 
    }
}