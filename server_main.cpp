#include <iostream>
#include "network/Server.h"

int main() {
    Server authServer(12345);
    authServer.run();

    return 0;
}
