#include "networking/TCPConnection.h"

#include <utils/log.h>
#include <boost/bind/bind.hpp>

namespace Core::Networking {
    TCPConnection::TCPConnection(io_context& context) {
        this->socket = new tcp::socket(context);
    }

    TCPConnection::~TCPConnection() {
        LOG_LINE("TCOConnection destructor");
        socket->shutdown(tcp::socket::shutdown_both);
        socket->close();
    }

    void TCPConnection::Start(MessageCallback&& msgCallback, ErrorCallback&& errorHandler) {
        messageCallback = std::move(msgCallback);
        errorCallback = std::move(errorHandler);
        this->StartRead();
    }

    void TCPConnection::PostPackage(ActualPackage &&package) {
        bool queueIdle = pendingPackages.empty();
        pendingPackages.push(std::move(package));

        if (queueIdle) this->StartSendPackage();
    }

    void TCPConnection::Post(const std::string &message) {
        bool queueIdle = pendingMessages.empty();
        pendingMessages.push(message);

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
        async_write(
            *socket,
            buffer(pendingMessages.top()),
            boost::bind(
                &TCPConnection::HandleWrite,
                shared_from_this(),
                placeholders::error,
                placeholders::bytes_transferred
            )
        );
    }

    void TCPConnection::StartSendPackage() {

    }

    void TCPConnection::HandleRead(const boost::system::error_code &ec, std::size_t bytesTransferred) {
        if (!ec) {
            std::string received(
                buffers_begin(streamBuffer.data()),
                buffers_begin(streamBuffer.data()) + bytesTransferred - 1
            );
            streamBuffer.consume(bytesTransferred);

            auto headerDelimiter = received.find(':');
            int bytesToRead = std::stoi(received.substr(0, headerDelimiter));
            Package::Type packageType = (Package::Type)std::stoi(received.substr(headerDelimiter + 1, 1));

            std::string data = received.substr(received.find('|') + 1, bytesToRead);

            switch (packageType) {
                case Package::Type::TextMessage:
                    messageCallback(data);
                    break;
                default:
                    break;
            }

            LOG_LINE(received);
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
            pendingMessages.pop();
            if (!pendingMessages.empty()) this->StartWrite();
        }
        else {
            LOG_LINE("HandleWrite " << ec.what());
            socket->close();
            errorCallback();
        }
    }
}
