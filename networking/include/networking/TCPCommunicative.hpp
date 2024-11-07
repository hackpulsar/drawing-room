#ifndef TCPCOMMUNICATIVE_HPP
#define TCPCOMMUNICATIVE_HPP

#include "TCPPackage.h"

namespace Core::Networking {
    using namespace boost::asio;
    using ip::tcp;

    typedef std::function<void(boost::system::error_code, std::size_t)> AsyncCallbackF;

    // Base class that implements basic sockets' communication.
    // Note that you should connect the socket yourself in a class
    // derived from this.
    class TCPCommunicative {
    public:
        TCPCommunicative() = default;
        virtual ~TCPCommunicative() { delete socket; };

        virtual bool SendPackage(const ActualPackage &package) {
            // Sending header with size of the body and package type.
            // Type is necessary for the server to parse the package correctly.
            auto e = this->SendString(
                std::to_string(package.header.bodySize) + ":" + std::to_string((int)package.header.type) + "|" +
                package.body.data + ";"
            );

            if (e) return false;
            return true;
        }

        // Does everything the same way except it's async.
        virtual void AsyncSendPackage(const ActualPackage &package) {
            this->AsyncSendString(
                std::to_string(package.header.bodySize) + ":" + std::to_string((int)package.header.type) + "|" +
                package.body.data + ";",
                [](boost::system::error_code ec, std::size_t bytes_transferred) {
                    // ...
                }
            );
        }

    protected:
        boost::system::error_code SendString(const std::string &message) const {
            boost::system::error_code ec;
            write(*socket, buffer(message), ec);
            return ec;
        }

        void AsyncSendString(const std::string &message, const AsyncCallbackF& callback) const {
            async_write(*socket, buffer(message), callback);
        }

        tcp::socket* socket{};
    };
}

#endif //TCPCOMMUNICATIVE_HPP
