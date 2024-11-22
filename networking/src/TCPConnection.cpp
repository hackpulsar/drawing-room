#include "networking/TCPConnection.h"

#include <utils/log.h>
#include <boost/bind/bind.hpp>

namespace Core::Networking {
    TCPConnection::TCPConnection(io_context& context) {
        this->socket = new tcp::socket(context);
    }

    TCPConnection::~TCPConnection() {
        LOG_LINE("TCPConnection destructor");
        socket->shutdown(tcp::socket::shutdown_both);
        socket->close();
    }

    void TCPConnection::SetID(std::size_t id) { this->id = id; }
    void TCPConnection::SetUsername(const std::string &username) { this->username = username; }

    std::size_t TCPConnection::GetID() const { return this->id; }
    const std::string &TCPConnection::GetUsername() const { return this->username; }

    void TCPConnection::Start(PackageCallback &&pckgCallback, ErrorCallback &&errorHandler) {
        packageCallback = std::move(pckgCallback);
        errorCallback = std::move(errorHandler);
        this->StartRead();
    }

    void TCPConnection::PostPackage(Package &&package) {
        bool queueIdle = pendingPackages.empty();
        pendingPackages.push(std::move(package));

        if (queueIdle) this->StartWrite();
    }

    void TCPConnection::Post(const Package &package) {
        bool queueIdle = pendingPackages.empty();
        pendingPackages.push(package);

        if (queueIdle) this->StartWrite();
    }

    tcp::socket& TCPConnection::getSocket() { return *socket; }

    void TCPConnection::StartRead() {
        // Reading until first ';' character, which indicates the end of the package.
        async_read_until(
            *socket,
            streamBuffer, ";",
            boost::bind(
                &TCPConnection::HandleRead,
                shared_from_this(),
                placeholders::error,
                placeholders::bytes_transferred
            )
        );
    }

    void TCPConnection::StartWrite() {
        this->AsyncSendPackage(
            pendingPackages.top(),
            boost::bind(
                &TCPConnection::HandleWrite,
                shared_from_this(),
                placeholders::error,
                placeholders::bytes_transferred
            )
        );
    }

    void TCPConnection::HandleRead(const boost::system::error_code &ec, std::size_t bytesTransferred) {
        if (!ec) {
            auto package = Package::Parse(streamBuffer, bytesTransferred);
            packageCallback(package);

            switch (package.getHeader().type) {
                case Package::Type::TextMessage:
                    LOG_LINE("Message from id " << package.getHeader().senderID << ": " << package.getBody().data);
                    break;
                case Package::Type::BoardUpdate:
                    LOG_LINE("Board update from id " << package.getHeader().senderID << ": " << package.getBody().data);
                    break;
                default:
                    break;
            }
        }
        else if (ec == error::eof) {
            // Disconnected correctly
            socket->close();
            errorCallback();
            return;
        }
        else {
            // Connection lost
            LOG_LINE(ec.what());
            socket->close();
            errorCallback();
            return;
        }

        this->StartRead();
    }

    void TCPConnection::HandleWrite(const boost::system::error_code &ec, std::size_t bytesTransferred) {
        if (!ec) {
            pendingPackages.pop();
            if (!pendingPackages.empty()) this->StartWrite();
        }
        else {
            LOG_LINE("HandleWrite " << ec.what());
            socket->close();
            errorCallback();
        }
    }
}
