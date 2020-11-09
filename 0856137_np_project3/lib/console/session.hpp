#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include "console.hpp"
#include <bits/stdc++.h>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#define MAX_LENGTH 4096
#define Max_session_num 5

using namespace std;
using namespace boost::asio;

struct SessionInfo {
    bool used;
    int id;
    string host;
    int port;
    string file;
};

class Session : public enable_shared_from_this<Session> {
private:
    boost::asio::io_service& io_service;
    ip::tcp::resolver resolver_;
    ip::tcp::socket socket_;
    ip::tcp::resolver::query query_;
    int id;
    int port;
    string host;
    string file;
    array<char, MAX_LENGTH> data_;
    // char data_[MAX_LENGTH];
    queue<string> cmd_que;
    enum State { start,
        read,
        write,
        wait } state;
    void LoadCommand();
    void Resolve();
    void Connect(ip::tcp::resolver::iterator it);
    void Shell();
    void do_read();
    void do_write();
    void output_cmd();
    void output_ret(string& ret, size_t length);

public:
    void Start();

    // Construct
    Session(boost::asio::io_service& io_service, SessionInfo& session)
        : io_service(io_service)
        , id(session.id)
        , port(session.port)
        , host(session.host)
        , file(session.file)
        , resolver_(io_service)
        , socket_(io_service)
        , query_(session.host, to_string(session.port)){};
};

// void _response() {
//     cout << "Content-type: text/html\n\n"
//          << "<!DOCTYPE html>\n"
//          << "<html lang=\"en\">\n";
// }
void response(SessionInfo* session);
void parse_query_string(SessionInfo* session, string query);
void escape(string& data);
