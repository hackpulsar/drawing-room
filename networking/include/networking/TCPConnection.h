#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "utils/settings.h"

namespace Core::Networking {
    using namespace boost::asio;
    using ip::tcp;

    typedef std::function<void(const std::string&)> MessageCallback;

    class TCPConnection : public boost::enable_shared_from_this<TCPConnection> {
    public:
        typedef boost::shared_ptr<TCPConnection> pointer;

        explicit TCPConnection(io_context& context);
        ~TCPConnection();

        static pointer Create(io_context& context) {
            return pointer(new TCPConnection(context));
        }

        void Start(MessageCallback&& msgCallback);

        void Post(const std::string& sMessage);

        tcp::socket& getSocket();

    private:
        void OnRead();
        void OnWrite();

        void HandleRead(const boost::system::error_code& ec, std::size_t nBytesTransferred);
        void HandleWrite(const boost::system::error_code& ec, std::size_t nBytesTransferred);

        tcp::socket m_Socket;
        streambuf m_Buffer { Settings::MESSAGE_MAX_SIZE };

        std::stack<std::string> m_PendingMessages;

        MessageCallback m_MessageCallback;

    };
}

#endif //TCPCONNECTION_H
