#include "networking/TCPServer.h"

int main() {
    Core::Networking::TCPServer server(1499);
    server.Run();
}