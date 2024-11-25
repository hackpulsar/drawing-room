#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "TCPCommunicative.hpp"
#include "utils/settings.h"

namespace Core::Networking {
    using namespace boost::asio;
    using ip::tcp;

    // Starting from 1, because 0 is server's ID.
    inline size_t GetNextConnectionID() {
        static size_t id = 1;
        return id++;
    }

    typedef std::function<void(const Package&)> PackageCallback;
    typedef std::function<void()> ErrorCallback;

    class TCPConnection :
        public boost::enable_shared_from_this<TCPConnection>,
        public TCPCommunicative {
    public:
        typedef boost::shared_ptr<TCPConnection> pointer;

        explicit TCPConnection(io_context& context);
        ~TCPConnection() override;

        static pointer Create(io_context& context) {
            return pointer(new TCPConnection(context));
        }

        void SetID(std::size_t id);
        void SetUsername(const std::string& username);

        std::size_t GetID() const;
        const std::string& GetUsername() const;

        void Start(PackageCallback&& pckgCallback, ErrorCallback&& errorCallback);

        void PostPackage(Package&& package);
        void Post(const Package &package);

        tcp::socket& getSocket();

    private:
        void StartRead();
        void StartWrite();

        void HandleRead(const boost::system::error_code& ec, std::size_t bytesTransferred);
        void HandleWrite(const boost::system::error_code& ec, std::size_t bytesTransferred);

        std::stack<Package> pendingPackages;

        PackageCallback packageCallback;
        ErrorCallback errorCallback;

        IDType id{};
        std::string username = "unknown";

    };
}

#endif //TCPCONNECTION_H
