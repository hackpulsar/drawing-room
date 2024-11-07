#ifndef TCPPACKAGE_H
#define TCPPACKAGE_H

namespace Core::Networking {
    namespace Package {
        enum class Type {
            TextMessage = 0,
            BoardUpdate
        };

        struct Header {
            std::size_t bodySize;
            Type type;
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

            auto headerDelimiter = received.find(':');
            int bytesToRead = std::stoi(received.substr(0, headerDelimiter));
            Package::Type packageType = (Package::Type)std::stoi(received.substr(headerDelimiter + 1, 1));

            std::string data = received.substr(received.find('|') + 1, bytesToRead);

            return ActualPackage {
                Package::Header { (std::size_t)bytesToRead, packageType },
                Package::Body { data }
            };
        }

        // Compresses the package into a string.
        // Used to sed package through the network.
        static std::string Compress(const ActualPackage& package) {
            return std::to_string(package.header.bodySize) +
                ":" + std::to_string((int)package.header.type) +
                "|" + package.body.data + ";";
        }
    };
}

#endif //TCPPACKAGE_H
