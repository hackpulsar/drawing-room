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

        // Received an ID.
        std::istream is(&streamBuffer);
        std::string message;
        std::getline(is, message);
        id = std::stoi(message);

        LOG_LINE("Received an ID from the server: " << id);

        return true;
    }

    void TCPClient::StartReading() {
        // Once again, ';' indicates the end of the package
        async_read_until(
            *socket,
            streamBuffer, ";",
            [this] (boost::system::error_code ec, size_t bytes_transferred) {
                this->OnPackageReceived(ec, bytes_transferred);
            }
        );
        context.run();
    }

    void TCPClient::Stop() {
        context.stop();
    }

    bool TCPClient::IsConnected() const { return connected; }

    void TCPClient::SetUsername(const std::string &username) { this->username = username; }

    std::size_t TCPClient::GetID() const { return id; }

    boost::system::error_code TCPClient::ReadStringUntil(char delimiter) {
        boost::system::error_code ec;
        read_until(*socket, streamBuffer, delimiter, ec);
        return ec;
    }

    void TCPClient::OnPackageReceived(const boost::system::error_code& ec, std::size_t bytesTransferred) {
        if (!ec) {
            auto package = ActualPackage::Parse(streamBuffer, bytesTransferred);

            switch (package.header.type) {
                case Package::Type::TextMessage:
                    msgRecCallback(package.body.data);
                    LOG_LINE(package.body.data);
                    break;
                case Package::Type::BoardUpdate:
                    pkgRecCallback(package);
                    break;
                default:
                    break;
            }

            if (this->IsConnected()) {
                this->StartReading();
            }
        }
        else
            LOG_LINE("Error receiving message.");
    }

}
