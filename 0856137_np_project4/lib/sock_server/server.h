#include <array>
#include <bits/stdc++.h>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>

#include "reply.hpp"
#include "request.hpp"
#include "request_handler.hpp"
#include "request_parser.hpp"
#include "socks4.hpp"

#define MAX_LENGTH 8196
using namespace std;
using namespace boost::asio;

bool CheckFirewall(string ip, string mode);

class SockSession : public enable_shared_from_this<SockSession> {
private:
    ip::tcp::socket client_socket_;
    ip::tcp::socket server_socket_;
    ip::tcp::acceptor acceptor_;
    ip::tcp::resolver resolver_;
    // ip::tcp::resolver::query query_;
    array<char, MAX_LENGTH> input_buffer;
    array<char, MAX_LENGTH> output_buffer;
    io_service& io_service_;
    socks4::request socks_request;

public:
    SockSession(const SockSession&) = delete;
    SockSession& operator=(const SockSession&) = delete;
    void start() { do_read_request(); }
    void stop() { client_socket_.close(); }
    explicit SockSession(ip::tcp::socket socket, boost::asio::io_service& io_service)
        : client_socket_(move(socket))
        , server_socket_(io_service)
        , acceptor_(io_service)
        , resolver_(io_service)
        , io_service_(io_service){};

private:
    void do_accept(socks4::reply socks_reply);
    void do_read_request();
    void do_connection(ip::tcp::resolver::iterator it);
    void do_resolve(ip::tcp::resolver::query query);
    void do_connect(ip::tcp::resolver::iterator it);
    void do_reply_client(socks4::reply socks_reply);
    void read_client();
    void write_client(size_t length);
    void read_server();
    void write_server(size_t length);
    void Handle_request(bool is_good_request);
};
