#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <boost/asio.hpp>

#include "TCPConnection.h"

namespace Core::Networking {
    enum class TCPServerState { OK = 0, ERROR = 1 };

    using namespace boost::asio;

    struct TCPConnection_Data {
        TCPConnection::pointer connection;
        size_t ID;
        std::string username;
    };

    class TCPServer {
    public:
        explicit TCPServer(int port);
        ~TCPServer();
        
        void StartAccept();

        void BroadcastMessage(const std::string& message) const;
        void Broadcast(const ActualPackage& package) const;

    private:
        void HandleAccept(TCPConnection_Data& connection, const boost::system::error_code& ec);

        int port;
        io_context IOContext;
        tcp::acceptor acceptor;

        std::vector<TCPConnection_Data> connections;

    };
}

#endif //TCPSERVER_H
