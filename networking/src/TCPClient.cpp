#include "networking/TCPClient.h"

#include <boost/bind/bind.hpp>

#include "utils/log.h"

namespace Core::Networking {
    TCPClient::TCPClient()
        : socket(IOContext) {
    }

    TCPClient::~TCPClient() {
        if (this->IsConnected())
            socket.shutdown(ip::tcp::socket::shutdown_both);
        socket.close();
    }


    boost::system::error_code TCPClient::ConnectTo(const std::string &address, const std::string &port) {
        using namespace boost::asio::ip;

        tcp::resolver resolver(IOContext);
        auto endpoint = resolver.resolve(address, port);

        boost::system::error_code ec;
        this->endpoint = connect(socket, endpoint, ec);

        if (!ec) connected = true;

        return ec;
    }

    void TCPClient::SendString(const std::string &message) {
        async_write(
            socket, buffer(message),
            [this](boost::system::error_code e, size_t bytes_transferred) {
                if (e) {
                    LOG_LINE("Error sending a message");
                    socket.close();
                }
            }
        );
    }

    void TCPClient::StartReading() {
        async_read_until(
            socket,
            streamBuffer, "\n",
            [this] (boost::system::error_code ec, size_t bytes_transferred) {
                this->OnMessageReceived(ec, bytes_transferred);
            }
        );
        IOContext.run();
    }

    bool TCPClient::IsConnected() const { return connected; }

    void TCPClient::OnMessageReceived(const boost::system::error_code& ec, std::size_t bytesTransferred) {
        if (!ec) {
            std::string message(
                buffers_begin(streamBuffer.data()),
                buffers_begin(streamBuffer.data()) + bytesTransferred - 1
            );
            streamBuffer.consume(bytesTransferred);

            LOG_LINE(message);

            msgRecCallback(message);
            if (this->IsConnected()) {
                this->StartReading();
            }
        }
        else
            LOG_LINE("Error receiving message.");
    }

}
