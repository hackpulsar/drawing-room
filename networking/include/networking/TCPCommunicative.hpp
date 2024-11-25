#ifndef TCPCOMMUNICATIVE_HPP
#define TCPCOMMUNICATIVE_HPP

#include "TCPPackage.h"
#include "utils/log.h"
#include "utils/settings.h"

namespace Core::Networking {
    using namespace boost::asio;
    using ip::tcp;

    typedef std::function<void(boost::system::error_code, std::size_t)> AsyncCallback;

    // Base class that implements basic sockets' communication.
    // Note that you should connect the socket yourself in a class
    // derived from this.
    class TCPCommunicative {
    public:
        TCPCommunicative() = default;
        virtual ~TCPCommunicative() { delete socket; };

        virtual bool SendPackage(const Package &package) {
            // Sending header with size of the body and package type.
            // Type is necessary for the server to parse the package correctly.
            auto e = this->SendString(Package::CompressToJSON(package).dump() + ";");

            if (e) return false;
            return true;
        }

        // Does everything the same way except it's async.
        void AsyncSendPackage(
            const Package &package,
            const AsyncCallback& callback = [](boost::system::error_code ec, std::size_t bytes_transferred) {}
        ) const {
            LOG_LINE(Package::CompressToJSON(package).dump() + ";");
            this->AsyncSendString(Package::CompressToJSON(package).dump() + ";", callback);
        }

    protected:
        boost::system::error_code SendString(const std::string &message) const {
            boost::system::error_code ec;
            write(*socket, buffer(message), ec);
            return ec;
        }

        boost::system::error_code ReadStringUntil(char delimiter) {
            boost::system::error_code ec;
            read_until(*socket, streamBuffer, delimiter, ec);
            return ec;
        }

        void AsyncSendString(const std::string &message, const AsyncCallback& callback) const {
            async_write(*socket, buffer(message), callback);
        }

        streambuf streamBuffer { Settings::MESSAGE_MAX_SIZE };
        tcp::socket* socket{};
    };
}

#endif //TCPCOMMUNICATIVE_HPP
