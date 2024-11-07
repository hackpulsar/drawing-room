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

    void TCPConnection::Start(PackageCallback &&pckgCallback, ErrorCallback &&errorHandler) {
        packageCallback = std::move(pckgCallback);
        errorCallback = std::move(errorHandler);
        this->StartRead();
    }

    void TCPConnection::PostPackage(ActualPackage &&package) {
        bool queueIdle = pendingPackages.empty();
        pendingPackages.push(std::move(package));

        if (queueIdle) this->StartWrite();
    }

    void TCPConnection::Post(const ActualPackage &package) {
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
            auto package = ActualPackage::Parse(streamBuffer, bytesTransferred);
            packageCallback(package);

            switch (package.header.type) {
                case Package::Type::TextMessage:
                    LOG_LINE(package.body.data);
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
