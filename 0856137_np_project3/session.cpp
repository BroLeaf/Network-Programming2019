#include "lib/console/session.hpp"
#include <bits/stdc++.h>

using namespace std;

void Session::Start()
{
    LoadCommand();
    Resolve();
}

void Session::LoadCommand()
{
    string cmdpath = "test_case/" + file;
    ifstream file_in;
    file_in.open(cmdpath, std::ifstream::in);
    string str;
    while (getline(file_in, str)) {
        cmd_que.push(str);
        // cout << str << flush;
    }
    return;
}

void Session::Resolve()
{
    // auto self(shared_from_this());
    resolver_.async_resolve(query_, [this](const boost::system::error_code& ec, ip::tcp::resolver::iterator it) {
        if (!ec) {
            Connect(it);
        } else {
            cerr << "Resolver error\n"
                 << flush;
        }
    });
    // cout << "<script>document.getElementById('s0').innerHTML += \"% \";</script>\n" << flush;
    // cout << "shell output\n" << endl << flush;
}

void Session::Connect(ip::tcp::resolver::iterator it)
{
    socket_.async_connect(it->endpoint(), [this](const boost::system::error_code& ec) {
        if (!ec) {
            cout.clear();
            // memset(data_, 0, MAX_LENGTH);
            do_read();
        } else {
            cerr << "Connection error\n"
                 << flush;
        }
    });
}

void Session::do_read()
{
    // memset(&data_, 0, sizeof(data_));
    auto self(shared_from_this());
    // usleep(10000);
    socket_.async_read_some(buffer(data_, MAX_LENGTH),
        [this, self](const boost::system::error_code& ec, size_t length) {
            if (!ec) {
                string ret(data_.data(), data_.data() + length);
                // cout << " length " << length << flush;
                // cout << data_ << endl << "\n" << flush;
                output_ret(ret, length);
                // memset(&data_, 0, length);
                if (ret.find("% ") != string::npos)
                    do_write();
                usleep(1000000);
                do_read();
            } else {
                cerr << "Read from host failed!" << endl
                     << flush;
            }
        });
}

void Session::do_write()
{
	auto self(shared_from_this());
    if (cmd_que.empty()) {
        return;
    }
    // usleep(10000);
    output_cmd();
    socket_.async_send(buffer(cmd_que.front() + "\n"),
        [this, self](const boost::system::error_code& ec, size_t length) {
            if (!ec) {
                cmd_que.pop();
                // cout << cmd_que.front();
                do_read();
            } else {
                cerr << "Write command to host failed!" << endl
                     << flush;
            }
        });
    // cmd_que.pop();
    // cout << "WWWW" << flush;
}

void Session::output_cmd()
{
    string cmd = cmd_que.front() + "\n";
    escape(cmd);
    cout << "<script>document.getElementById(\"s" + to_string(id) + "\").innerHTML += '<b>" + cmd + "';</script>" << endl
         << flush;
    cout.clear();
}

void Session::output_ret(string& ret, size_t length)
{
    /* escape */
    // cout << " In output " << endl;
    // string res (data_, 0, length);
    escape(ret);
    cout << "<script>document.getElementById(\"s" + to_string(id) + "\").innerHTML += '" + ret + "</b>';</script>" << flush;
    cout.clear();
}

void escape(string& data)
{
    string buffer;
    buffer.reserve(data.size());
    for (size_t pos = 0; pos != data.size(); ++pos) {
        switch (data[pos]) {
        case '&':
            buffer.append("&amp;");
            break;
        case '\"':
            buffer.append("&quot;");
            break;
        case '\'':
            buffer.append("&apos;");
            break;
        case '<':
            buffer.append("&lt;");
            break;
        case '>':
            buffer.append("&gt;");
            break;
        case '\n':
            buffer.append("&NewLine;");
            break;
        case '\r':
            buffer.append("");
            break;
        case '\t':
            buffer.append("&emsp");
            break;
        default:
            buffer.append(&data[pos], 1);
            break;
        }
    }
    data.swap(buffer);
}
