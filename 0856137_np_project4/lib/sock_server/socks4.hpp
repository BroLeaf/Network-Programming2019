#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>

using namespace std;
using namespace boost::asio;

namespace socks4 {

// VN=4, CD=1 or 2
class request {
public:
    enum command_type {
        connect = 0x01,
        bind = 0x02
    };

    request()
        : null_byte_(0)
        , user_id_("")
        , domain_name("")
    {
    }

    boost::array<mutable_buffer, 2056> buffers()
    {
        boost::array<mutable_buffer, 2056> bufs = {
            { buffer(&version_, 1),
                buffer(&command_, 1),
                buffer(&port_high_byte_, 1),
                buffer(&port_low_byte_, 1),
                buffer(address_),
                buffer(unprocessed, 2048) }
        };
        return bufs;
    }

    void process_userid_domain_name()
    {
        int term = 1;
        for (int i = 0; i < 2048; i++) {
            if (unprocessed[i] != '\0' && term == 1) {
                user_id_ += unprocessed[i];
            } else if (unprocessed[i] != '\0' && term == 2) {
                domain_name += unprocessed[i];
            } else
                term++;
            if (term == 3)
                break;
        }
    }

    string get_domain_name()
    {
        return domain_name;
    }

    string get_user_id()
    {
        return user_id_;
    }

    unsigned char command() const { return command_; }

    /*string user_id() const {
            string uid;
            uid.append(user_id_.data(), user_id_.data()+10);

            return uid;
        }*/

    string get_command() const
    {
        if (command_ == connect)
            return "CONNECT";
        else if (command_ == bind)
            return "BIND";
        else
            return "*** ERROR command ***";
    }

    ip::tcp::endpoint endpoint() const
    {
        unsigned short port = port_high_byte_;
        port = static_cast<unsigned short>((port << 8) & 0xff00);
        port = port | port_low_byte_;

        ip::address_v4 address(address_);

        return ip::tcp::endpoint(address, port);
    }

    void printenv() const
    {
        cout << "<D_IP>\t\t:" << this->endpoint().address() << endl;
        cout << "<D_PORT>\t:" << this->endpoint().port() << endl;
        cout << "<Command>\t:" << this->get_command() << endl;
    }

private:
    unsigned char version_;
    unsigned char command_;
    unsigned char port_high_byte_;
    unsigned char port_low_byte_;
    ip::address_v4::bytes_type address_;
    string user_id_;
    string domain_name;
    char unprocessed[2048];
    // boost::array<mutable_buffer, 1024> user_id_;
    // boost::array<mutable_buffer, 1024> domain_name;

    unsigned char null_byte_;
};

// VN=0, CD=90(accepted) or 91(rejected or failed)
class reply {
public:
    enum status_type {
        request_granted = 0x5a,
        request_failed = 0x5b
        //request_failed_no_identd = 0x5c,
        //request_failed_bad_user_id = 0x5d
    };

    reply(status_type status, const ip::tcp::endpoint& endpoint)
        : status_(status)
        , null_byte_(0)
    {
        // Only IPv4 is supported by the SOCKS 4 protocol.
        if (endpoint.protocol() != ip::tcp::v4()) {
            throw boost::system::system_error(
                error::address_family_not_supported);
        }

        // Convert port number to network byte order.
        unsigned short port = endpoint.port();
        port_high_byte_ = static_cast<unsigned char>((port >> 8) & 0xff);
        port_low_byte_ = static_cast<unsigned char>(port & 0xff);

        // Save IP address in network byte order.
        address_ = endpoint.address().to_v4().to_bytes();
    }

    unsigned char status() const { return status_; }

    boost::array<const_buffer, 5> buffers() const
    {
        boost::array<const_buffer, 5> bufs = {
            { buffer(&null_byte_, 1),
                buffer(&status_, 1),
                buffer(&port_high_byte_, 1),
                buffer(&port_low_byte_, 1),
                buffer(address_) }
        };
        return bufs;
    }

private:
    unsigned char null_byte_;
    unsigned char status_;
    unsigned char port_high_byte_;
    unsigned char port_low_byte_;
    ip::address_v4::bytes_type address_;
};
}
