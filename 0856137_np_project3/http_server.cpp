#define BOOST_FILESYSTEM_VERSION 3
#include <bits/stdc++.h>
#include <boost/asio.hpp>
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include "lib/http_server/connection.hpp"
#include "lib/http_server/connection_manager.hpp"
#include "lib/http_server/header.hpp"
#include "lib/http_server/request_handler.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
using namespace std;
using namespace boost::asio;
io_service global_io_service;

namespace http {
namespace server {

    void connection::do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(
            buffer(data_, MAX_LENGTH),
            [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    // cout << string(_data.data(), _data.data() + bytes_transferred) << endl;
                    request_parser::result_type result;
                    std::tie(result, std::ignore) = request_parser_.Parse(
                        request_, data_.data(), data_.data() + length);
                    if (result == request_parser::good) {
                        // request_handler_.handle_request(request_, reply_);
                        Handle_request(true);
                    } else if (result == request_parser::bad) {
                        // reply_ = reply::stock_reply(reply::bad_request);
                        Handle_request(false);
                    } else {
                        do_read();
                    }
                } else if (ec != error::operation_aborted) {
                    connection_manager_.stop(shared_from_this());
                }
            });
    }
    void connection::Handle_request(bool is_good_request)
    {
        auto self(shared_from_this());
        boost::filesystem::path execfile;
        string response_msg;

        size_t pos = request_.uri.find_first_of("?");
        if (pos == string::npos) {
            request_.file_path = request_.uri;
        } else {
            request_.file_path = request_.uri.substr(0, pos);
            request_.query_string = request_.uri.substr(pos + 1);
        }
        cout << "file_path is : " << request_.file_path << " is " << (is_good_request ? "good" : "bad") << endl
             << "query_string is : " << ((!request_.query_string.empty())?request_.query_string:"(query_string is empty.)") << endl;

        if (is_good_request) {
            execfile = boost::filesystem::current_path() / request_.file_path;
            // cout << execfile.generic_string() << endl;
            if (boost::filesystem::is_regular_file(execfile)) {
                response_msg += "HTTP/1.1 200 OK\r\n";
            } else {
                response_msg += "HTTP/1.1 404 Not Found\r\n"
                                "Content-Type: text/html\n\n"
                                "<html>\n"
                                "<head><title>404 Not Found</title></head>\n"
                                "<body><h1>404 Not Found</h1></body>\n"
                                "</html>\n";
            }
        } else {
            response_msg += "HTTP/1.0 400 Bad Request\r\n"
                            "Content-Type: text/html\n\n"
                            "<html>\n"
                            "<head><title>400 Not Found</title></head>\n"
                            "<body><h1>400 Not Found</h1></body>\n"
                            "</html>\n";
        }
        socket_.async_send(
            buffer(response_msg, response_msg.size()),
            [this, self, is_good_request, execfile](boost::system::error_code ec,
                size_t length) {
                // cout << request_.method << endl << request_.http_version_major << endl << request_.http_version_minor << endl;
                if (!ec && is_good_request && boost::filesystem::is_regular_file(execfile)) {
                    clearenv();
                    setenv("REQUEST_METHOD", request_.method.c_str(), 1);
                    setenv("REQUEST_URI", request_.uri.c_str(), 1);
                    setenv("QUERY_STRING", request_.query_string.c_str(), 1);
                    string http_protocal = "HTTP/" + to_string(request_.http_version_major) + "."
                        + to_string(request_.http_version_minor);
                    setenv("SERVER_PROTOCAL", http_protocal.c_str(), 1);
                    string host_name;
                    for (auto& it : request_.headers) {
                        // cout << it.name << " " << it.value << endl;
                        if (it.name == "Host") {
                            size_t pos = it.value.find_first_of(":");
                            host_name = it.value.substr(0, pos);
                            break;
                        }
                    }
                    // cout << request_.query_string << endl;
                    setenv("HTTP_HOST", host_name.c_str(), 1);
                    setenv("SERVER_ADDR", socket_.local_endpoint().address().to_string().c_str(), 1);
                    cout << to_string(socket_.local_endpoint().port()) << endl;
                    setenv("SERVER_PORT", to_string(socket_.local_endpoint().port()).c_str(), 1);
                    setenv("REMOTE_ADDR", socket_.remote_endpoint().address().to_string().c_str(), 1);
                    setenv("REMOTE_PORT", to_string(socket_.remote_endpoint().port()).c_str(), 1);
                    global_io_service.notify_fork(io_service::fork_prepare);
                    pid_t pid = fork();
                    if (pid == 0) {

                        global_io_service.notify_fork(io_service::fork_child);
                        dup2(socket_.native_handle(), STDIN_FILENO);
                        dup2(socket_.native_handle(), STDOUT_FILENO);
                        dup2(socket_.native_handle(), STDERR_FILENO);
                        connection_manager_.stop_all();
                        boost::system::error_code ignore;
                        socket_.shutdown(ip::tcp::socket::shutdown_both, ignore);
                        /* acceptor not close */
                        if (execlp(execfile.c_str(), execfile.c_str(), NULL) < 0)
                            cout << "Content-type:text/html\r\n\r\n<h1>Execute failed</h1>\r\n";
                    } else {
                        global_io_service.notify_fork(io_service::fork_parent);
                        socket_.close();
                    }
                }
                if (ec != error::operation_aborted) {
                    connection_manager_.stop(self);
                }
            });
    }

    class HttpServer {
    private:
        ip::tcp::acceptor acceptor_;
        ip::tcp::socket socket_;
        connection_manager connection_manager_;

    public:
        HttpServer(short port)
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
                    connection_manager_.start(make_shared<connection>(
                        move(socket_), connection_manager_, global_io_service));
                }
                do_accept();
                return 0;
            });
        }
    };
}
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
        http::server::HttpServer server(port);
        global_io_service.run();
    } catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
