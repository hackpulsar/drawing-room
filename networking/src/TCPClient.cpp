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

    bool TCPClient::SendPackage(const ActualPackage& package) {
        // Sending header with size of the body and package type.
        // Type is necessary for the server to parse the package correctly.
        auto e = this->SendString(
            std::to_string(package.header.bodySize) + ":" + std::to_string((int)package.header.type) + ";"
        );
        if (e) return false;

        // Sending the body
        e = this->SendString(package.body.data);
        if (e) return false;

        return true;
    }

    // TODO: implement reading packages from the server
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

    void TCPClient::Stop() {
        IOContext.stop();
    }

    bool TCPClient::IsConnected() const { return connected; }

    void TCPClient::SetUsername(const std::string &username) { this->username = username; }

    boost::system::error_code TCPClient::SendString(const std::string &message) {
        boost::system::error_code ec;
        write(socket, buffer(message), ec);
        return ec;
    }

    void TCPClient::AsyncSendString(const std::string &message) {
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

    boost::system::error_code TCPClient::ReadStringUntil(char delimiter) {
        boost::system::error_code ec;
        read_until(socket, streamBuffer, delimiter, ec);
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
