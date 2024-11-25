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
        if (socket->is_open())
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

    bool TCPClient::Handshake(bool loadTheCanvas) {
        // Construct a handshake package
        nlohmann::json data;
        data["username"] = username;

        Package handshake {
            Package::Header{ data.dump().length(), Package::Type::Handshake, -1 },
            Package::Body{ data }
        };
        if (!this->SendPackage(handshake))
            return false;

        if (this->ReadStringUntil(';'))
            return false;

        if (loadTheCanvas) {
            // TODO: start accepting packages with lines
            // NOTE: number of packages will be in the response
        }

        // Received an ID.
        std::istream is(&streamBuffer);
        std::string handshakeResponse;
        std::getline(is, handshakeResponse);
        id = Package::Parse(handshakeResponse.substr(0, handshakeResponse.size() - 1)).getBody().data.at("id");

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

    void TCPClient::OnPackageReceived(const boost::system::error_code& ec, std::size_t bytesTransferred) {
        if (!ec) {
            auto package = Package::Parse(streamBuffer, bytesTransferred);
            pkgRecCallback(package);

            if (this->IsConnected()) {
                this->StartReading();
            }
        }
        else {
            LOG_LINE("Error receiving message. " << ec.what());
        }
    }

}
