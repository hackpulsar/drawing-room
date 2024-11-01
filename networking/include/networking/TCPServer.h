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

        void Broadcast(const std::string& sMessage) const;

    private:
        void HandleAccept(const TCPConnection_Data& connection, const boost::system::error_code& ec);

        int m_nPort;
        io_context m_IOContext;
        tcp::acceptor m_Acceptor;

        std::vector<TCPConnection_Data> m_Connections;

    };
}

#endif //TCPSERVER_H
