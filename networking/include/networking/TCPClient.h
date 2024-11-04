#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <boost/asio.hpp>

#include "TCPPackage.h"
#include "utils/settings.h"

namespace Core::Networking {
    using namespace boost::asio;

    typedef std::function<void(const std::string&)> MessageReceivedCallback;

    class TCPClient {
    public:
        explicit TCPClient();
        ~TCPClient();

        boost::system::error_code ConnectTo(const std::string& address, const std::string& port);
        bool Handshake();

        bool SendPackage(const ActualPackage& package);

        void StartReading();
        void Stop();

        bool IsConnected() const;

        void SetUsername(const std::string& username);

        MessageReceivedCallback msgRecCallback;

    private:
        boost::system::error_code SendString(const std::string& message);
        void AsyncSendString(const std::string& message);

        boost::system::error_code ReadStringUntil(char delimiter);

        void OnMessageReceived(const boost::system::error_code& ec, std::size_t bytesTransferred);

        io_context IOContext {};

        ip::tcp::endpoint endpoint;
        ip::tcp::socket socket;

        streambuf streamBuffer { Settings::MESSAGE_MAX_SIZE };
        bool connected = false;
        std::string username;
    };
}

#endif //TCPCLIENT_H
