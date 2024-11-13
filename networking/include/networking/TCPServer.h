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

        void BroadcastMessage(const std::string& message, IDType sender) const;
        void BroadcastToEach(const ActualPackage& package) const;
        void BroadcastToEachExcept(const ActualPackage& package, IDType except) const;

    private:
        void HandleAccept(TCPConnection::pointer& connection, const boost::system::error_code& ec);

        int port;
        io_context IOContext;
        tcp::acceptor acceptor;

        std::vector<TCPConnection::pointer> connections;

    };
}

#endif //TCPSERVER_H
