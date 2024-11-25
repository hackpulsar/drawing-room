#include "networking/TCPServer.h"

#include <boost/bind/bind.hpp>

#include "utils/log.h"

namespace Core::Networking {
    TCPServer::TCPServer(int port)
        : port(port), acceptor(IOContext, tcp::endpoint(ip::tcp::v4(), port))
    { }

    TCPServer::~TCPServer() {
        // Close all connections
        LOG_LINE("Server shutdown");
        for (auto& c : connections) {
            c->getSocket().shutdown(tcp::socket::shutdown_both);
            c->getSocket().close();
        }
        connections.clear();
    }

    void TCPServer::Run() {
        this->StartAccept();
        LOG_LINE("Server is UP");
        IOContext.run();
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

    void TCPServer::BroadcastMessage(const std::string &message, IDType sender) const {
        std::string senderUsername = sender == 0 ? "Server" : "unknown";
        // Getting a username based on sender's ID.
        for (auto& c : connections) {
            if (c->GetID() == sender)
                senderUsername = c->GetUsername();
        }

        nlohmann::json data;
        data["message"] = senderUsername + ": " + message;
        this->BroadcastToEach(Package {
            Package::Header { message.size() + senderUsername.size() + 2, Package::Type::TextMessage, sender },
            Package::Body { data }
        });
    }

    void TCPServer::BroadcastToEach(const Package &package) const {
        for (auto& c : connections) {
            if (c->getSocket().is_open())
                c->Post(package);
        }
    }

    void TCPServer::BroadcastToEachExcept(const Package &package, IDType except) const {
        for (auto& c : connections) {
            if (c->GetID() != except && c->getSocket().is_open())
                c->Post(package);
        }
    }

    void TCPServer::HandleAccept(TCPConnection::pointer& connection, const boost::system::error_code& ec) {
        if (!ec) {
            // Reading handshake package
            std::string handshakeBuff;
            boost::system::error_code e;
            read_until(connection->getSocket(), dynamic_buffer(handshakeBuff), ";", e);

            if (e) {
                LOG_LINE("Reading handshake request failed.");
                return;
            }

            // Parsing trimmed handshake buffer (removed ';')
            Package handshakePkg = Package::Parse(handshakeBuff.substr(0, handshakeBuff.size() - 1));
            connection->SetUsername(handshakePkg.getBody().data.at("username"));

            nlohmann::json data;
            data["id"] = connection->GetID();
            Package handshakeResponse {
                Package::Header{ data.dump().length(), Package::Type::Handshake, Settings::SERVER_ID },
                Package::Body{ data }
            };

            // Sending back user's ID.
            write(connection->getSocket(), buffer(Package::CompressToJSON(handshakeResponse).dump() + ";"), e);

            if (e) {
                LOG_LINE("Sending handshake response failed.");
                return;
            }

            LOG_LINE("Connection established with user " << "\'" << connection->GetUsername() << "\', id: " << connection->GetID());

            connection->Start(
                [this](const Package &package) {
                    if (package.getHeader().type == Package::Type::TextMessage) {
                        // Transforming the message. Adding sender username then broadcasting.
                        this->BroadcastMessage(package.getBody().data.at("message"), package.getHeader().senderID);
                    }
                    else
                        this->BroadcastToEachExcept(package, package.getHeader().senderID);
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
            this->BroadcastMessage("User " + connection->GetUsername() + " has joined.\n", 0);
        }
        else
            LOG_LINE(ec.what());

        this->StartAccept();
    }

}