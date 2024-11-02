#include "networking/TCPServer.h"

#include "utils/log.h"
#include <boost/bind/bind.hpp>

namespace Core::Networking {
    TCPServer::TCPServer(int port)
        : port(port), acceptor(IOContext, tcp::endpoint(ip::tcp::v4(), port))
    {
        this->StartAccept();
        IOContext.run();
    }

    TCPServer::~TCPServer() {
        // Close all connections
        LOG_LINE("Server shutdown");
        for (auto& s : connections) {
            s.connection->getSocket().shutdown(tcp::socket::shutdown_both);
            s.connection->getSocket().close();
        }
    }

    void TCPServer::StartAccept() {
        TCPConnection::pointer newConnection = TCPConnection::Create(IOContext);
        TCPConnection_Data newConnectionData = { newConnection, GetNextConnectionID() };

        acceptor.async_accept(
            newConnection->getSocket(),
            boost::bind(
                &TCPServer::HandleAccept,
                this, newConnectionData,
                placeholders::error
            )
        );

        connections.push_back(newConnectionData);
    }

    void TCPServer::Broadcast(const std::string &message) const {
        for (auto& c : connections) {
            if (c.connection->getSocket().is_open())
                c.connection->Post(message);
        }
    }

    void TCPServer::HandleAccept(TCPConnection_Data& connection, const boost::system::error_code& ec) {
        if (!ec) {
            // Reading username
            std::array<char, 128> usernameBuff {};
            std::streamsize len = (std::streamsize)connection.connection->getSocket().read_some(buffer(usernameBuff));
            std::string username = std::string(usernameBuff.data(), len);

            connection.username = username;
            LOG_LINE("Connection established with user " << "\'" << username << "\'");

            // Sending welcome message
            std::string welcomeMessage = "[Server]: Welcome, " + username + "!\n";
            write(connection.connection->getSocket(), buffer(welcomeMessage));
            LOG_LINE("Welcome sent");

            connection.connection->Start(
                [this, connection](const std::string& message) {
                    this->Broadcast(connection.username + ": " + message + "\n");
                },
                [this, connection]() {
                    if (this->connections.erase(
                        std::find_if(
                            connections.begin(), connections.end(),
                            [this, connection](const TCPConnection_Data& c) {
                                return c.ID == connection.ID;
                            }
                        )
                    ) != connections.end()) {
                        this->Broadcast("User " + connection.username + " has left.\n");
                        LOG_LINE("User " + connection.username + " has left.\n");
                    }
                }
            );

            // Broadcasting new connection
            this->Broadcast("User " + username + " has joined.\n");
        }
        else
            LOG_LINE(ec.what());

        this->StartAccept();
    }

}