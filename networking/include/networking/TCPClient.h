#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <boost/asio.hpp>

#include "TCPCommunicative.hpp"
#include "utils/settings.h"

namespace Core::Networking {
    using namespace boost::asio;

    typedef std::function<void(const Package&)> PackageReceivedCallback;

    class TCPClient : public TCPCommunicative {
    public:
        TCPClient();
        ~TCPClient() override;

        boost::system::error_code ConnectTo(const std::string& address, const std::string& port);
        bool Handshake(bool loadTheCanvas = false);

        void StartReading();
        void Stop();

        bool IsConnected() const;

        void SetUsername(const std::string& username);

        std::size_t GetID() const;

        PackageReceivedCallback pkgRecCallback;

    private:
        void OnPackageReceived(const boost::system::error_code& ec, std::size_t bytesTransferred);

        io_context context{};
        tcp::endpoint endpoint;

        bool connected = false;
        std::string username;
        IDType id{};
    };
}

#endif //TCPCLIENT_H
