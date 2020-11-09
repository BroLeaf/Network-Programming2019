#define BOOST_FILESYSTEM_VERSION 3
#include <boost/asio.hpp>
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
// #include "lib/console/console.hpp"
#include "lib/console/session.hpp"
#include <bits/stdc++.h>
#include <unistd.h>
#include <utility>

using namespace std;
using namespace boost::asio;
io_service global_io_service;

int main()
{
    // _response();
    SessionInfo sessions[Max_session_num];
    parse_query_string(sessions, getenv("QUERY_STRING"));
    // cout << sessions[2].used << "\r";
    response(sessions);
    set<shared_ptr<Session>> session_set;
    for (int i = 0; i < Max_session_num; ++i) {
        // cout << i << endl;
        if (sessions[i].used) {
        	auto ptr = make_shared<Session>(global_io_service, sessions[i]);
        	session_set.insert(ptr);
        	ptr->Start();
		}
    }
    global_io_service.run();
    // cout << "IO is already run." << endl;
    return 0;
}

void parse_query_string(SessionInfo* session, string query)
{
    for (int i = 0; i < Max_session_num && !query.empty(); ++i) {
        // cout << query << "\r";
        size_t pos;
        pos = query.find_first_of("&");
        // cout << query.substr(3, pos-3);
        session[i].id = i;
        session[i].host = query.substr(3, pos - 3);
        query.erase(0, pos + 1);
        pos = query.find_first_of("&");
        session[i].port = atoi(query.substr(3, pos - 3).c_str()); // string to int may cause crash
        query.erase(0, pos + 1);
        pos = query.find_first_of("&");
        session[i].file = query.substr(3, pos - 3);
        query.erase(0, pos + 1);
        if (session[i].host.empty() || session[i].port == 0 || session[i].file.empty())
            session[i].used = false;
        else
            session[i].used = true;
        // cout << session[i].host << " " << session[i].port << " " << session[i].file << "\r";
    }
}

void response(SessionInfo* session)
{
    string response_msg = "Content-Type: text/html\n\n"
                          "<!DOCTYPE html>\n"
                          "<html lang=\"en\">\n"
                          "  <head>\n"
                          "    <meta charset=\"UTF-8\" />\n"
                          "    <title>NP Project 3 Console</title>\n"
                          "    <link\n"
                          "      rel=\"stylesheet\"\n"
                          "      href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\n"
                          "      integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\n"
                          "      crossorigin=\"anonymous\"\n"
                          "    />\n"
                          "    <link\n"
                          "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
                          "      rel=\"stylesheet\"\n"
                          "    />\n"
                          "    <link\n"
                          "      rel=\"icon\"\n"
                          "      type=\"image/png\"\n"
                          "      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n"
                          "    />\n"
                          "    <style>\n"
                          "      * {\n"
                          "        font-family: 'Source Code Pro', monospace;\n"
                          "        font-size: 1rem !important;\n"
                          "      }\n"
                          "      body {\n"
                          "        background-color: #212529;\n"
                          "      }\n"
                          "      pre {\n"
                          "        color: #cccccc;\n"
                          "      }\n"
                          "      b {\n"
                          "        color: #ffffff;\n"
                          "      }\n"
                          "    </style>\n"
                          "  </head>\n"
                          "  <body>\n"
                          "    <table class=\"table table-dark table-bordered\">\n"
                          "      <thead>\n"
                          "        <tr>\n";
    for (int i = 0; i < Max_session_num; i++) {
		if (session[i].used) {
        	response_msg += "<th scope=\"col\">" + session[i].host + ":"
            	+ to_string(session[i].port) + "</th>\n";
		}
    }
    response_msg += "        </tr>\n"
                    "      </thead>\n"
                    "      <tbody>\n"
                    "        <tr>\n";
    for (int i = 0; i < Max_session_num; i++) {
		if (session[i].used) {
        	response_msg += "<td><pre id=\"s" + to_string(i) + "\" class=\"mb-0\">"
            	+ "</pre></td>";
		}
    }
    response_msg += "         </tr>\n"
                    "       </tbody>\n"
                    "     </table>\n"
                    "  </body>\n"
                    "</html>\n";
    cout << response_msg << endl
         << flush;
    cout.clear();
}
