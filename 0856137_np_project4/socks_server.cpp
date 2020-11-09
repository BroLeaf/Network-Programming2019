#include "lib/sock_server/header.hpp"
#include "lib/sock_server/request_handler.hpp"
#include "lib/sock_server/server.h"
#include <bits/stdc++.h>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
using namespace std;
using namespace boost::asio;
io_service global_io_service;

void SockSession::do_read_request()
{
    auto self(shared_from_this());
    client_socket_.async_read_some(socks_request.buffers(), [this, self](boost::system::error_code ec, size_t length) {
        if (!ec) {
            socks_request.process_userid_domain_name();
            // cout << "domain name: " << socks_request.get_domain_name() << endl;

            ip::tcp::endpoint dst_endpoint = socks_request.endpoint();
            string dst_ip = dst_endpoint.address().to_string();
            if (dst_ip.find("0.0.0.") == string::npos) {
                ip::tcp::resolver::query query_(dst_ip, to_string(dst_endpoint.port()));
                do_resolve(query_);
            } else {
                ip::tcp::resolver::query query_(socks_request.get_domain_name(), to_string(dst_endpoint.port()));
                do_resolve(query_);
            }
        }
    });
    return;
}

void SockSession::do_resolve(ip::tcp::resolver::query query)
{
    auto self(shared_from_this());
    resolver_.async_resolve(query, [this, self](boost::system::error_code ec, ip::tcp::resolver::iterator it) {
        if (!ec) {
			boost::asio::ip::tcp::endpoint end = *it;
			bool FireWall_out = CheckFirewall(end.address().to_string(), socks_request.get_command());
            string dst_ip = end.address().to_string();
            cout << "<S_IP>: " << client_socket_.remote_endpoint().address() << endl
                 << flush;
            cout << "<S_PORT>: " << client_socket_.remote_endpoint().port() << endl
                 << flush;
            if (dst_ip == "::1")
                dst_ip = "0.0.0.0";
            cout << "<D_IP>: " << dst_ip << endl
                 << flush;
            cout << "<D_PORT>: " << end.port() << endl
                 << flush;
            cout << "<Command>: " << socks_request.get_command() << endl
                 << flush;
            if (FireWall_out) {
                cout << "<Reply>: "
                     << "Accept" << endl
                     << flush;
                cout << "----------------------------------" << endl
                     << flush;
                switch (socks_request.command()) {
                case 1: {           //CONNECT mode
                    do_connection(it);
                    break;
                }
                case 2: {           // BIND mode
                    unsigned short port = 0;
                    ip::tcp::endpoint bind_endPoint(ip::tcp::endpoint(ip::tcp::v4(), port));
                    acceptor_.open(bind_endPoint.protocol());
                    acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));
                    acceptor_.bind(bind_endPoint);
                    acceptor_.listen();
                    socks4::reply socks_reply(socks4::reply::request_granted, acceptor_.local_endpoint());
                    write(client_socket_, socks_reply.buffers());
                    do_accept(socks_reply);
                    break;
                }
                }
            } else {
                cout << "<Reply>: "
                     << "Reject" << endl
                     << flush;
                cout << "----------------------------------" << endl
                     << flush;
                socks4::reply reply_(socks4::reply::request_failed, socks_request.endpoint());
                do_reply_client(reply_);
                client_socket_.close();
            }

            
        } else
            cerr << "resolve error" << endl;
    });
}

void SockSession::do_accept(socks4::reply socks_reply)
{
    auto self(shared_from_this());
    acceptor_.async_accept(server_socket_, [this, self, socks_reply](boost::system::error_code ec) {
        if (!ec) {
            do_reply_client(socks_reply);
        } else {
            cerr << "bind error" << endl;
            client_socket_.close();
        }
    });
}

void SockSession::do_reply_client(socks4::reply socks_reply)
{
    // return;
    auto self(shared_from_this());
    async_write(client_socket_, socks_reply.buffers(), [this, self, socks_reply](boost::system::error_code ec, size_t) {
        if (!ec) {
            // cout << "Redirect message success" << endl;
            if (socks_reply.status() == socks4::reply::request_granted) {
                read_client();
                read_server();
            } else {
                client_socket_.close();
                server_socket_.close();
            }

        } else {
            cerr << "Send reply error" << endl
                 << flush;
            client_socket_.close();
            server_socket_.close();
        }
    });
}

void SockSession::do_connection(ip::tcp::resolver::iterator it)
{
    auto self(shared_from_this());
    server_socket_.async_connect(*it, [this, self](boost::system::error_code ec) {
        if (!ec) {

            // cout << "Connection success" << endl << flush;
            socks4::reply socks_reply(socks4::reply::request_granted, socks_request.endpoint());
            do_reply_client(socks_reply);
        } else {
            // cerr << "Connection to destination error!" << endl;
            socks4::reply socks_reply(socks4::reply::request_failed, socks_request.endpoint());
            do_reply_client(socks_reply);
        }
    });
}

void SockSession::read_client()
{
    auto self(shared_from_this());
    client_socket_.async_read_some(buffer(input_buffer), [this, self](boost::system::error_code ec, size_t length) {
        if (!ec) {
            // string request;
            // request.append(input_buffer.data(), input_buffer.data()+length);
            // cout << "request from client:" << request << endl;
            write_server(length);
        } else {
            // cerr << "read from client error" << endl;
            client_socket_.close();
            server_socket_.close();
        }
    });
}

void SockSession::write_server(size_t size)
{
    auto self(shared_from_this());
    async_write(server_socket_, buffer(input_buffer, size),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                read_client();
            } else {
                // cerr << "send message to server error, close socket and connection" << endl;
                client_socket_.close();
                server_socket_.close();
            }
        });
}

void SockSession::read_server()
{
    auto self(shared_from_this());
    server_socket_.async_read_some(buffer(output_buffer), [this, self](boost::system::error_code ec, size_t length) {
        if (!ec) {
            // string request;
            // request.append(output_buffer.data(), output_buffer.data()+length);
            // cout << "receive from server:" << request << endl;
            write_client(length);
        } else {
            // cerr << "read from server error" << endl;
            client_socket_.close();
            server_socket_.close();
        }
    });
}

void SockSession::write_client(size_t size)
{
    auto self(shared_from_this());
    async_write(client_socket_, buffer(output_buffer, size), [this, self](boost::system::error_code ec, size_t length) {
        if (!ec) {
            read_server();
        } else {
            // cerr << "send message to client error, close socket and connection" << endl;
            client_socket_.close();
            server_socket_.close();
        }
    });
    // client_socket_.async_send(buffer(output_buffer, size), [this, self](boost::system::error_code ec, size_t length) {
    //     if (!ec) {
    //         read_server();
    //     } else {
    //         // cerr << "send message to client error, close socket and connection" << endl;
    //         client_socket_.close();
    //         server_socket_.close();
    //     }
    // });
}

class SockServer {
private:
    ip::tcp::acceptor acceptor_;
    ip::tcp::socket socket_;

public:
    SockServer(short port)
        : acceptor_(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port))
        , socket_(global_io_service)
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
            if (!ec) {
                // cout << "fork here" << endl;
                global_io_service.notify_fork(io_service::fork_prepare);
                pid_t pid = fork();
                if (pid == 0) {
                    // child
                    global_io_service.notify_fork(io_service::fork_child);
                    acceptor_.close();
                    make_shared<SockSession>(move(socket_), global_io_service)->start();
                } else {
                    // parent
                    global_io_service.notify_fork(io_service::fork_parent);
                    socket_.close();
                    do_accept();
                }
            } else {
                cerr << "accept error:" << ec.message() << endl;
                do_accept();
            }
        });
    }
};

bool CheckFirewall(string ip, string mode)
{
    // return true;
    string line;
    ifstream firewall_conf("socks.conf");
    // check if the config_file exists
    if (firewall_conf.fail()) {
        cerr << "error: socks.conf not found\n";
        return -1;
    }
    // cout << ip;
    string mode_str = (mode == "CONNECT") ? "c" : "b";
    // cout << mode_str << endl << flush;

    if (firewall_conf) {
        while (getline(firewall_conf, line)) {
            // each line have same format:
            /* permit b/c *.*.*.*       # comment */
            vector<string> store_rule;
            istringstream istr(line);
            string str;
            while (istr >> str)
                store_rule.push_back(str);
            // cout << store_rule[1] << " " << store_rule[2] << endl << flush;
            if (store_rule[1] == mode_str) {
                string dst_ip = ip, rule_ip = store_rule[2];
                while (!dst_ip.empty() && !rule_ip.empty()) {
                    if (rule_ip[0] == '*')
                        return true;
                    else if (dst_ip[0] == rule_ip[0]) {
                        dst_ip.erase(dst_ip.begin());
                        rule_ip.erase(rule_ip.begin());
                    } else if (dst_ip[0] != rule_ip[0])
                        break;
                    // cout << "ip is:\t\t" << dst_ip << endl;
                    // cout << "rule is:\t" << rule_ip << endl << flush;
                }
                if (dst_ip.empty() && rule_ip.empty())
                    return true;
            }
        }
        firewall_conf.close();
    }
    return false;
}

int main(int argc, char* const argv[])
{
    signal(SIGCHLD, SIG_IGN);
    if (argc != 2) {
        cerr << "Usage:" << argv[0] << " [port]" << endl;
        return 1;
    }

    try {
        unsigned short port = atoi(argv[1]);
        SockServer SockServer(port);
        global_io_service.run();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
