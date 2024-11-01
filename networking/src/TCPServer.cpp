#include "networking/TCPServer.h"

#include "utils/log.h"
#include <boost/bind/bind.hpp>

namespace Core::Networking {
    TCPServer::TCPServer(int port)
        : m_nPort(port), m_Acceptor(m_IOContext, tcp::endpoint(ip::tcp::v4(), port))
    {
        this->StartAccept();
        m_IOContext.run();
    }

    TCPServer::~TCPServer() {
        // Close all connections
        LOG_LINE("Server shutdown");
        for (auto& s : m_Connections) {
            s.pConnection->getSocket().shutdown(tcp::socket::shutdown_both);
            s.pConnection->getSocket().close();
        }
    }

    void TCPServer::StartAccept() {
        TCPConnection::pointer newConnection = TCPConnection::Create(m_IOContext);
        TCPConnection_Data newConnectionData = { newConnection, GetNextConnectionID() };

        m_Acceptor.async_accept(
            newConnection->getSocket(),
            boost::bind(
                &TCPServer::HandleAccept,
                this, newConnectionData,
                placeholders::error
            )
        );

        m_Connections.push_back(newConnectionData);
    }

    void TCPServer::Broadcast(const std::string &sMessage) const {
        for (auto& c : m_Connections) {
            if (c.pConnection->getSocket().is_open())
                c.pConnection->Post(sMessage);
        }
    }

    void TCPServer::HandleAccept(const TCPConnection_Data& connection, const boost::system::error_code& ec) {
        if (!ec) {
            // Reading username
            std::array<char, 128> usernameBuff {};
            std::streamsize len = (std::streamsize)connection.pConnection->getSocket().read_some(buffer(usernameBuff));
            std::string sUsername = std::string(usernameBuff.data(), len);

            LOG_LINE("Connection established with user " << "\'" << sUsername << "\'");

            // Sending welcome message
            std::string sWelcomeMessage = "Welcome, " + sUsername + "!\n";
            write(connection.pConnection->getSocket(), buffer(sWelcomeMessage));
            LOG_LINE("Welcome sent");

            // Broadcasting new connection
            this->Broadcast("User " + sUsername + " has joined\n");

            connection.pConnection->Start(
                [this](const std::string& sMessage) {
                    this->Broadcast(sMessage + "\n");
                },
                [this, connection, &sUsername]() {
                    if (this->m_Connections.erase(
                        std::find_if(
                            m_Connections.begin(), m_Connections.end(),
                            [this, connection](const TCPConnection_Data& c) {
                                return c.ID == connection.ID;
                            }
                        )
                    ) != m_Connections.end()) {
                        this->Broadcast("User " + sUsername + " has left.\n");
                    }
                }
            );
        }
        else
            LOG_LINE(ec.what());

        this->StartAccept();
    }

}