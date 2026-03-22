#ifndef PACKET_H
#define PACKET_H

#include <string>
#include <nlohmann/json.hpp>

enum class PacketType {
    SERVER_SEND_POOL,    // The Server tells the Client what Word choices it rolled
    SERVER_UPDATE_HP,    // The Server broadcasts updated HP/Shield stats after a turn resolves
    CLIENT_SUBMIT_TURN,  // A Client submits their built Sentence combo to the Server
    GAME_OVER,           // The Server announces a winner
    SERVER_LOG_MESSAGE,  // The Server broadcasts custom error strings to be displayed!
    CLIENT_LOGIN,        // UI streams distinct {username, password} keys to interceptor port
    SERVER_LOGIN_RESPONSE, // Server bounces success/fail boolean
    SERVER_SEND_LEADERBOARD, // Drops a calculated Top 5 array string 
    SERVER_MATCH_FOUND,  // Server broadcasts matched opponent identity
    CLIENT_PLAY_REQUEST, // Exits the idle Menu loop into active logic
    CLIENT_REGISTER,     // Client explicitly signals database initialization 
    CLIENT_CANCEL_MATCHMAKING // Formally completely seamlessly natively abandons dynamic natively queued arrays securely!
};

// Represents a network payload over TCP
struct Packet {
    PacketType type;
    
    nlohmann::json payload;

    // Serialization: Converts our struct into a flat standard string to send over wire.
    std::string dump() const {
        nlohmann::json j;
        j["type"] = static_cast<int>(type);
        j["payload"] = payload;
        return j.dump() + "\n"; 
    }

    // Deserialization: Converts the received flat string back into a useful Packet struct.
    static Packet parse(const std::string& data) {
        auto j = nlohmann::json::parse(data);
        return Packet{ static_cast<PacketType>(j["type"].get<int>()), j["payload"] };
    }
};

#endif // PACKET_H
