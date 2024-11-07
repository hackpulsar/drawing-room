#include "networking/TCPServer.h"

#include <boost/bind/bind.hpp>

#include "utils/log.h"

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
        for (auto& c : connections) {
            c->getSocket().shutdown(tcp::socket::shutdown_both);
            c->getSocket().close();
        }
        connections.clear();
    }

    void TCPServer::StartAccept() {
        TCPConnection::pointer newConnection = TCPConnection::Create(IOContext);
        newConnection->SetID(GetNextConnectionID());

        acceptor.async_accept(
            newConnection->getSocket(),
            boost::bind(
                &TCPServer::HandleAccept,
                this, newConnection,
                placeholders::error
            )
        );

        connections.push_back(newConnection);
    }

    void TCPServer::BroadcastMessage(const std::string &message, std::size_t sender) const {
        std::string senderUsername = sender == 0 ? "Server" : "unknown";
        // Getting a username based on sender's ID.
        for (auto& c : connections) {
            if (c->GetID() == sender)
                senderUsername = c->GetUsername();
        }

        this->Broadcast(ActualPackage {
            Package::Header { message.size() + senderUsername.size() + 2, Package::Type::TextMessage, sender },
            Package::Body { senderUsername + ": " + message }
        });
    }

    void TCPServer::Broadcast(const ActualPackage &package) const {
        for (auto& c : connections) {
            if (c->getSocket().is_open())
                c->Post(package);
        }
    }

    void TCPServer::HandleAccept(TCPConnection::pointer& connection, const boost::system::error_code& ec) {
        if (!ec) {
            // Reading username
            std::array<char, 128> usernameBuff {};
            std::streamsize len = (std::streamsize)connection->getSocket().read_some(buffer(usernameBuff));
            std::string username = std::string(usernameBuff.data(), len);

            connection->SetUsername(username);

            // Sending back user's ID.
            write(connection->getSocket(), buffer(std::to_string(connection->GetID()) + "\n"));
            LOG_LINE("Connection established with user " << "\'" << connection->GetUsername() << "\', id: " << connection->GetID());

            connection->Start(
                [this](const ActualPackage &package) {
                    if (package.header.type == Package::Type::TextMessage) {
                        // Transforming the message. Adding sender username then broadcasting.
                        this->BroadcastMessage(package.body.data, package.header.sender);
                    }
                    else
                        this->Broadcast(package);
                },
                [this, connection]() {
                    if (this->connections.erase(
                        std::find_if(
                            connections.begin(), connections.end(),
                            [this, connection](const TCPConnection::pointer& c) {
                                return c->GetID() == connection->GetID();
                            }
                        )
                    ) != connections.end()) {
                        this->BroadcastMessage("User " + connection->GetUsername() + " has left.\n", 0);
                        LOG_LINE("User " + connection->GetUsername() + " has left.\n");
                    }
                }
            );

            // Broadcasting new connection
            this->BroadcastMessage("User " + username + " has joined.\n", 0);
        }
        else
            LOG_LINE(ec.what());

        this->StartAccept();
    }

}