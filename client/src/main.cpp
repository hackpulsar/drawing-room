#include <boost/asio.hpp>

#include "utils/log.h"

int main() {
    boost::asio::io_context ctx;
    boost::asio::ip::tcp::resolver resolver(ctx);
    auto endpoint = resolver.resolve({"127.0.0.1", "1488"});

    boost::asio::ip::tcp::socket socket(ctx);
    boost::asio::connect(socket, endpoint);

    boost::system::error_code ec;

    // Sending username to a server
    LOG("Username: ");
    std::string sUsername;
    std::getline(std::cin, sUsername);

    boost::asio::write(socket, boost::asio::buffer(sUsername), ec);

    // Reading response
    std::array<char, 128> buffer {};
    std::streamsize len = (std::streamsize)socket.read_some(boost::asio::buffer(buffer), ec);

    if (!ec)
        std::cout.write(buffer.data(), len);
    else
        LOG_LINE(ec.what());

    boost::asio::streambuf buff;
    bool bServerOpen = true;

    // Launching reading thread
    std::thread readingThread([&]() {
        while (true) {
            read_until(socket, buff, "\n");
            if (!ec) {
                std::istream is(&buff);
                std::string message;
                std::getline(is, message);
                LOG_LINE(message);
            }
            else if (ec == boost::asio::error::eof) {
                bServerOpen = false;
                break;
            }
            else
                break;
        }
    });

    std::string sMessage;
    do {
        if (socket.is_open() == false) break;

        if (bServerOpen) {
            std::getline(std::cin, sMessage);
            boost::asio::write(socket, boost::asio::buffer(sUsername + ": " + sMessage + "\n"), ec);
        }
    } while(sMessage != "/exit" && bServerOpen);

    return 0;
}
