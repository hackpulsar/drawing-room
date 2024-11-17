#ifndef TCPPACKAGE_H
#define TCPPACKAGE_H

#include "nlohmann/json.hpp"

namespace Core::Networking {
    typedef std::size_t IDType;

    class Package {
    public:
        enum class Type {
            TextMessage = 0,
            BoardUpdate
        };

        struct Header {
            std::size_t bodySize;
            Type type;
            IDType senderID;
        };

        struct Body {
            nlohmann::json data;
        };

        Package(Header header, Body body)
            : header(header), body(std::move(body)) { }

        Header getHeader() const { return header; }
        Body getBody() const { return body; }

        static nlohmann::json CompressToJSON(const Package& package) {
            nlohmann::json compressed;

            compressed["header"]["bodySize"] = package.header.bodySize;
            compressed["header"]["type"] = package.header.type;
            compressed["header"]["senderID"] = package.header.senderID;

            compressed["body"]["data"] = package.body.data;

            return compressed;
        }

        static Package Parse(boost::asio::streambuf& buff, std::size_t bytesReceived) {
            using namespace boost::asio;

            std::string received(
                buffers_begin(buff.data()),
                buffers_begin(buff.data()) + bytesReceived - 1
            );
            buff.consume(bytesReceived);

            nlohmann::json receivedJSON = nlohmann::json::parse(received);
            return Package {
                Header {
                    receivedJSON.at("header").at("bodySize"),
                    receivedJSON.at("header").at("type"),
                    receivedJSON.at("header").at("senderID"),
                },
                Body {
                    receivedJSON.at("body").at("data")
                }
            };
        }

    protected:
        Header header = {};
        Body body = {};
    };
}

#endif //TCPPACKAGE_H
