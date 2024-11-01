#include "networking/TCPClient.h"

#include <utils/log.h>

namespace Core::Networking {
    TCPClient::TCPClient() : socket(IOContext) {
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

    boost::system::error_code TCPClient::SendString(const std::string &message) {
        boost::system::error_code ec;

        /*if (!connected) {
            ec.message = "TCP client is not connected";
            return ec;
        }*/

        async_write(
            socket, buffer(message),
            [&](boost::system::error_code e, size_t bytes_transferred) {
                if (e) {
                    LOG_LINE("Error sending a message");
                    socket.close();
                }
            }
        );

        return ec;
    }

    bool TCPClient::IsConnected() const { return connected; }

}
