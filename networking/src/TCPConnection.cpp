#include "networking/TCPConnection.h"

#include <utils/log.h>
#include <boost/bind/bind.hpp>

namespace Core::Networking {
    TCPConnection::TCPConnection(io_context& context)
        : socket(context)
    {

    }

    TCPConnection::~TCPConnection() {
        socket.shutdown(tcp::socket::shutdown_both);
        socket.close();
    }

    void TCPConnection::Start(MessageCallback&& msgCallback, ErrorCallback&& errorHandler) {
        messageCallback = std::move(msgCallback);
        errorCallback = std::move(errorHandler);
        this->OnRead();
    }

    void TCPConnection::Post(const std::string &message) {
        bool queueIdle = pendingMessages.empty();
        pendingMessages.push(message);

        if (queueIdle) this->OnWrite();
    }

    tcp::socket& TCPConnection::getSocket() { return socket; }

    void TCPConnection::OnRead() {
        async_read_until(
            socket,
            streamBuffer, "\n",
            boost::bind(
                &TCPConnection::HandleRead,
                shared_from_this(),
                placeholders::error,
                placeholders::bytes_transferred
            )
        );
    }

    void TCPConnection::OnWrite() {
        async_write(
            socket,
            buffer(pendingMessages.top()),
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
            std::string message(
                buffers_begin(streamBuffer.data()),
                buffers_begin(streamBuffer.data()) + bytesTransferred - 1
            );
            streamBuffer.consume(bytesTransferred);

            if (message == "/exit") {
                socket.shutdown(tcp::socket::shutdown_both);
                socket.close();
                return;
            }

            LOG_LINE(message);
            messageCallback(message);
        }
        else if (ec == error::eof) {
            errorCallback();
            socket.close();
            return;
        }
        else {
            LOG_LINE(ec.what());
            socket.close();
            return;
        }

        this->OnRead();
    }

    void TCPConnection::HandleWrite(const boost::system::error_code &ec, std::size_t bytesTransferred) {
        if (!ec) {
            pendingMessages.pop();
            if (!pendingMessages.empty()) this->OnWrite();
        }
        else {
            LOG_LINE("HandleWrite " << ec.what());
            socket.close();
        }
    }
}
