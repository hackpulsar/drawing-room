#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <boost/asio.hpp>

namespace Core::Networking {
    using namespace boost::asio;

    typedef std::function<void()> MessageReceivedCallback;

    class TCPClient {
    public:
        TCPClient();
        ~TCPClient();

        boost::system::error_code ConnectTo(const std::string& address, const std::string& port);
        boost::system::error_code SendString(const std::string& message);

        bool IsConnected() const;

    private:
        io_context IOContext;
        ip::tcp::endpoint endpoint;
        ip::tcp::socket socket;

        bool connected = false;

        std::string username;
    };
}

#endif //TCPCLIENT_H
