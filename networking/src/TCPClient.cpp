#include "networking/TCPClient.h"

#include <boost/bind/bind.hpp>

#include "utils/log.h"

namespace Core::Networking {
    TCPClient::TCPClient() {
        socket = new tcp::socket(context);
    }

    TCPClient::~TCPClient() {
        if (this->IsConnected())
            socket->shutdown(ip::tcp::socket::shutdown_both);
        socket->close();
    }

    boost::system::error_code TCPClient::ConnectTo(const std::string &address, const std::string &port) {
        using namespace boost::asio::ip;

        tcp::resolver resolver(context);
        auto endpoint = resolver.resolve(address, port);

        boost::system::error_code ec;
        this->endpoint = connect(*socket, endpoint, ec);

        if (!ec) connected = true;

        return ec;
    }

    bool TCPClient::Handshake() {
        if(this->SendString(username)) return false;

        if (this->ReadStringUntil('\n'))
            return false;

        // Received a string
        std::istream is(&streamBuffer);
        std::string message;
        std::getline(is, message);
        msgRecCallback(message);

        return true;
    }

    // TODO: implement reading packages from the server
    void TCPClient::StartReading() {
        async_read_until(
            *socket,
            streamBuffer, "\n",
            [this] (boost::system::error_code ec, size_t bytes_transferred) {
                this->OnMessageReceived(ec, bytes_transferred);
            }
        );
        context.run();
    }

    void TCPClient::Stop() {
        context.stop();
    }

    bool TCPClient::IsConnected() const { return connected; }

    void TCPClient::SetUsername(const std::string &username) { this->username = username; }

    boost::system::error_code TCPClient::ReadStringUntil(char delimiter) {
        boost::system::error_code ec;
        read_until(*socket, streamBuffer, delimiter, ec);
        return ec;
    }

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
