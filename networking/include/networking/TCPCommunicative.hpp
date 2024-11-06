#ifndef TCPCOMMUNICATABLE_H
#define TCPCOMMUNICATABLE_H

#include "TCPPackage.h"

#include "utils/log.h"

namespace Core::Networking {
    using namespace boost::asio;
    using ip::tcp;

    // Base class that implements basic sockets' communication.
    // Note that you should connect the socket yourself in the class
    // derived from this.
    class TCPCommunicative {
    public:
        TCPCommunicative() : socket(nullptr) { }
        virtual ~TCPCommunicative() { LOG_LINE("deleting..."); delete socket; };

        virtual bool SendPackage(const ActualPackage &package) {
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

    protected:
        boost::system::error_code SendString(const std::string &message) {
            boost::system::error_code ec;
            write(*socket, buffer(message), ec);
            return ec;
        }

        tcp::socket* socket;
    };
}

#endif //TCPCOMMUNICATABLE_H
