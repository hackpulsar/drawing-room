#ifndef TCPPACKET_H
#define TCPPACKET_H

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
    };
}

#endif //TCPPACKET_H
