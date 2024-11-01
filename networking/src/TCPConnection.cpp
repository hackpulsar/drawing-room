#include "networking/TCPConnection.h"

#include <utils/log.h>
#include <boost/bind/bind.hpp>

namespace Core::Networking {
    TCPConnection::TCPConnection(io_context& context)
        : m_Socket(context)
    {

    }

    TCPConnection::~TCPConnection() {
        m_Socket.shutdown(tcp::socket::shutdown_both);
        m_Socket.close();
    }

    void TCPConnection::Start(MessageCallback&& msgCallback, ErrorHandler&& errorHandler) {
        m_MessageCallback = std::move(msgCallback);
        m_ErrorCallback = std::move(errorHandler);
        this->OnRead();
    }

    void TCPConnection::Post(const std::string &sMessage) {
        bool bQueueIdle = m_PendingMessages.empty();
        m_PendingMessages.push(sMessage);

        if (bQueueIdle) this->OnWrite();
    }

    tcp::socket& TCPConnection::getSocket() { return m_Socket; }

    void TCPConnection::OnRead() {
        async_read_until(
            m_Socket,
            m_Buffer, "\n",
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
            m_Socket,
            buffer(m_PendingMessages.top()),
            boost::bind(
                &TCPConnection::HandleWrite,
                shared_from_this(),
                placeholders::error,
                placeholders::bytes_transferred
            )
        );
    }

    void TCPConnection::HandleRead(const boost::system::error_code &ec, std::size_t nBytesTransferred) {
        if (!ec) {
            std::string message(
                buffers_begin(m_Buffer.data()),
                buffers_begin(m_Buffer.data()) + nBytesTransferred - 1
            );
            m_Buffer.consume(nBytesTransferred);

            if (message == "/exit") {
                m_Socket.shutdown(tcp::socket::shutdown_both);
                m_Socket.close();
                return;
            }

            LOG_LINE(message);
            m_MessageCallback(message);
        }
        else if (ec == error::eof) {
            m_ErrorCallback();
            m_Socket.close();
            return;
        }
        else {
            LOG_LINE(ec.what());
            m_Socket.close();
            return;
        }

        this->OnRead();
    }

    void TCPConnection::HandleWrite(const boost::system::error_code &ec, std::size_t nBytesTransferred) {
        if (!ec) {
            m_PendingMessages.pop();
            if (!m_PendingMessages.empty()) this->OnWrite();
        }
        else {
            LOG_LINE("HandleWrite " << ec.what());
            m_Socket.close();
        }
    }
}
