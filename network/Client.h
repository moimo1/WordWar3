#ifndef CLIENT_H
#define CLIENT_H

#if defined(_WIN32)
  #define NOGDI
  #define NOUSER
#endif

#include <asio.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include "Packet.h"

class Client {
private:
    asio::io_context io_context;
    asio::ip::tcp::socket socket;
    
    // Background listener thread
    std::thread listenerThread;
    bool isRunning;

    // Mutex-protected queue to safely pass data from the network thread to the GUI thread
    std::queue<Packet> inbox;
    std::mutex inboxMutex;

    void listenLoop();

public:
    Client();
    ~Client();

    bool connectToServer(const std::string& ip, short port);
    
    // GUI safely calls this to send data
    void sendPacket(const Packet& packet);

    // GUI polls this every frame
    bool hasPacket();
    Packet popPacket();
};

#endif // CLIENT_H
