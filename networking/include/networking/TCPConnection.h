#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "utils/settings.h"

namespace Core::Networking {
    using namespace boost::asio;
    using ip::tcp;

    inline size_t GetNextConnectionID() {
        static size_t id = 0;
        return ++id;
    }

    typedef std::function<void(const std::string&)> MessageCallback;
    typedef std::function<void()> ErrorCallback;

    class TCPConnection : public boost::enable_shared_from_this<TCPConnection> {
    public:
        typedef boost::shared_ptr<TCPConnection> pointer;

        explicit TCPConnection(io_context& context);
        ~TCPConnection();

        static pointer Create(io_context& context) {
            return pointer(new TCPConnection(context));
        }

        void Start(MessageCallback&& msgCallback, ErrorCallback&& errorCallback);

        void Post(const std::string& message);

        tcp::socket& getSocket();

    private:
        void OnRead();
        void OnWrite();

        void HandleRead(const boost::system::error_code& ec, std::size_t bytesTransferred);
        void HandleWrite(const boost::system::error_code& ec, std::size_t bytesTransferred);

        tcp::socket socket;
        streambuf streamBuffer { Settings::MESSAGE_MAX_SIZE };

        std::stack<std::string> pendingMessages;

        MessageCallback messageCallback;
        ErrorCallback errorCallback;

    };
}

#endif //TCPCONNECTION_H
