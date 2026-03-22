#include "Client.h"
#include <iostream>

using asio::ip::tcp;

Client::Client() : io_context(), socket(io_context), isRunning(false) {
}

Client::~Client() {
    isRunning = false;
    asio::error_code ec;
    socket.close(ec);
    
    if (listenerThread.joinable()) {
        listenerThread.join();
    }
}

bool Client::connectToServer(const std::string& ip, short port) {
    try {
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(ip, std::to_string(port));
        
        asio::connect(socket, endpoints);
        std::cout << "Connected to Server at " << ip << ":" << port << "!\n";

        // Spin up the background thread to constantly wait for Server messages
        isRunning = true;
        listenerThread = std::thread(&Client::listenLoop, this);
        return true;

    } catch (std::exception& e) {
        std::cerr << "Failed to connect to server: " << e.what() << "\n";
        return false;
    }
}

void Client::sendPacket(const Packet& packet) {
    std::string data = packet.dump();
    try {
        asio::write(socket, asio::buffer(data));
    } catch (std::exception& e) {
        std::cerr << "Network Send Error: " << e.what() << "\n";
    }
}

void Client::listenLoop() {
    while (isRunning) {
        try {
            asio::streambuf buffer;
            // Blocks quietly in the background until the server drops a newline \n packet
            asio::read_until(socket, buffer, '\n');
            
            std::istream is(&buffer);
            std::string line;
            std::getline(is, line);
            
            Packet received = Packet::parse(line);

            // Safely lock the mailbox and drop the message in for the GUI
            std::lock_guard<std::mutex> lock(inboxMutex);
            inbox.push(received);

        } catch (std::exception& e) {
            std::cerr << "Disconnected from server or read error: " << e.what() << "\n";
            isRunning = false;
            break;
        }
    }
}

bool Client::hasPacket() {
    std::lock_guard<std::mutex> lock(inboxMutex);
    return !inbox.empty();
}

Packet Client::popPacket() {
    std::lock_guard<std::mutex> lock(inboxMutex);
    Packet p = inbox.front();
    inbox.pop();
    return p;
}
