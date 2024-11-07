#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <boost/asio.hpp>

#include "TCPConnection.h"

namespace Core::Networking {
    enum class TCPServerState { OK = 0, ERROR = 1 };

    using namespace boost::asio;

    class TCPServer {
    public:
        explicit TCPServer(int port);
        ~TCPServer();
        
        void StartAccept();

        void BroadcastMessage(const std::string& message, std::size_t sender) const;
        void Broadcast(const ActualPackage& package) const;

    private:
        void HandleAccept(TCPConnection::pointer& connection, const boost::system::error_code& ec);

        int port;
        io_context IOContext;
        tcp::acceptor acceptor;

        std::vector<TCPConnection::pointer> connections;

    };
}

#endif //TCPSERVER_H
