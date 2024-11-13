#ifndef TCPPACKAGE_H
#define TCPPACKAGE_H

namespace Core::Networking {
    typedef std::size_t IDType;

    namespace Package {
        enum class Type {
            TextMessage = 0,
            BoardUpdate
        };

        struct Header {
            std::size_t bodySize;
            Type type;
            IDType sender;
        };

        struct Body {
            std::string data;
        };
    }

    struct ActualPackage {
        ActualPackage(const Package::Header header, Package::Body body)
            : header(header), body(std::move(body))
        {}

        Package::Header header;
        Package::Body body;

        // Parses the buffer with a string representation of the package and
        // return a new ActualPackage instance.
        static ActualPackage Parse(boost::asio::streambuf& buff, std::size_t bytesReceived) {
            using namespace boost::asio;

            std::string received(
                buffers_begin(buff.data()),
                buffers_begin(buff.data()) + bytesReceived - 1
            );
            buff.consume(bytesReceived);

            auto bytesDelimiter = received.find_first_of(':');
            auto senderDelimiter = received.find_first_of(':', bytesDelimiter + 1);
            auto headerDelimiter = received.find('|');

            int bytesToRead = std::stoi(received.substr(0, bytesDelimiter));
            Package::Type packageType = (Package::Type)std::stoi(received.substr(bytesDelimiter + 1, 1));
            std::size_t sender = std::stoi(
                received.substr(
                    senderDelimiter + 1,
                    headerDelimiter - senderDelimiter
                )
            );
            std::string data = received.substr( headerDelimiter + 1, bytesToRead);

            return ActualPackage {
                Package::Header { (std::size_t)bytesToRead, packageType, sender },
                Package::Body { data }
            };
        }

        // Compresses the package into a string.
        // Used to sed package through the network.
        static std::string Compress(const ActualPackage& package) {
            return std::to_string(package.header.bodySize) +
                ":" + std::to_string((int)package.header.type) +
                ":" + std::to_string((int)package.header.sender) +
                "|" + package.body.data + ";";
        }
    };
}

#endif //TCPPACKAGE_H
