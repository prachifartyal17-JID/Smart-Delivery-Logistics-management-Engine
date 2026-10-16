// HttpServer.h  (Ashutosh) - minimal HTTP server on raw sockets. No external library.
// Serves .html/.css/.js files from the project folder and hands /api/... requests to a handler.
// Listens on localhost only. One request at a time (fine for a demo).
#pragma once
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <unistd.h>
#endif

namespace net {
#ifdef _WIN32
using Socket = SOCKET;
inline bool valid(Socket s) { return s != INVALID_SOCKET; }
inline void closeSocket(Socket s) { closesocket(s); }
#else
using Socket = int;
inline bool valid(Socket s) { return s >= 0; }
inline void closeSocket(Socket s) { ::close(s); }
#endif
}  // namespace net

struct Request  { std::string method, path, body; };
struct Response {
    int status = 200;
    std::string contentType = "application/json";
    std::string body;
    std::string location;                            // used by redirects
};

// RAII: the socket is closed when this object goes out of scope.
class Connection {
public:
    explicit Connection(net::Socket s) : s_(s) {}
    ~Connection() { if (net::valid(s_)) net::closeSocket(s_); }
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    net::Socket get() const { return s_; }
private:
    net::Socket s_;
};

class HttpServer {
public:
    using ApiHandler = std::function<Response(const Request&)>;

    HttpServer(int port, std::string rootDir, ApiHandler api)
        : port_(port), root_(std::move(rootDir)), api_(std::move(api)) {
#ifdef _WIN32
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) throw std::runtime_error("WSAStartup failed");
#endif
    }
    ~HttpServer() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    void run() {
        Connection listener(::socket(AF_INET, SOCK_STREAM, 0));
        if (!net::valid(listener.get())) throw std::runtime_error("Could not create socket");

        int yes = 1;
        ::setsockopt(listener.get(), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(static_cast<unsigned short>(port_));
        if (::bind(listener.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
            throw std::runtime_error("Could not use port " + std::to_string(port_) + " (already in use?)");
        if (::listen(listener.get(), 16) != 0) throw std::runtime_error("listen() failed");

        std::cout << "Hub running at http://localhost:" << port_ << "   (Ctrl+C to stop)\n";
        while (true) {
            Connection client(::accept(listener.get(), nullptr, nullptr));
            if (net::valid(client.get())) serve(client.get());
        }
    }

private:
    static constexpr std::size_t MAX_REQUEST = 1 << 20;   // 1 MB
    int port_;
    std::string root_;
    ApiHandler api_;

    static std::string lower(std::string s) {
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }
    static bool endsWith(const std::string& s, const std::string& tail) {
        return s.size() >= tail.size() && s.compare(s.size() - tail.size(), tail.size(), tail) == 0;
    }
    static Response text(int status, const std::string& message) {
        Response r;
        r.status = status;
        r.contentType = "text/plain; charset=utf-8";
        r.body = message;
        return r;
    }
    static const char* reason(int status) {
        switch (status) {
            case 200: return "OK";
            case 302: return "Found";
            case 400: return "Bad Request";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 413: return "Payload Too Large";
            default:  return "Internal Server Error";
        }
    }

    static void setTimeout(net::Socket s) {          // an idle browser connection must not freeze the server
#ifdef _WIN32
        DWORD ms = 2000;
        ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&ms), sizeof(ms));
#else
        timeval tv{2, 0};
        ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    }

    // 0 = parsed OK, -1 = nothing usable arrived, otherwise an HTTP error status
    static int readRequest(net::Socket s, Request& req) {
        std::string data;
        char buf[4096];
        std::size_t headerEnd = std::string::npos;
        while (headerEnd == std::string::npos) {
            int n = static_cast<int>(::recv(s, buf, sizeof(buf), 0));
            if (n <= 0) return -1;
            data.append(buf, static_cast<std::size_t>(n));
            if (data.size() > MAX_REQUEST) return 413;
            headerEnd = data.find("\r\n\r\n");
        }
        const std::string head = data.substr(0, headerEnd);
        std::istringstream line(head.substr(0, head.find("\r\n")));
        std::string target, version;
        line >> req.method >> target >> version;
        if (req.method.empty() || target.empty() || target[0] != '/') return 400;
        req.path = target.substr(0, target.find('?'));

        std::size_t length = 0;
        const std::size_t cl = lower(head).find("\r\ncontent-length:");
        if (cl != std::string::npos) length = std::strtoul(head.c_str() + cl + 17, nullptr, 10);
        if (length > MAX_REQUEST) return 413;

        std::string body = data.substr(headerEnd + 4);
        while (body.size() < length) {
            int n = static_cast<int>(::recv(s, buf, sizeof(buf), 0));
            if (n <= 0) return -1;
            body.append(buf, static_cast<std::size_t>(n));
        }
        req.body = body.substr(0, length);
        return 0;
    }

    Response serveStatic(const Request& req) const {
        if (req.method != "GET") return text(405, "Method not allowed");
        if (req.path == "/") {
            Response r;
            r.status = 302;
            r.location = "/ashutosh/index.html";
            return r;
        }
        if (req.path.find("..") != std::string::npos || req.path.find('\\') != std::string::npos) return text(403, "Forbidden");

        std::string type;
        if (endsWith(req.path, ".html"))     type = "text/html; charset=utf-8";
        else if (endsWith(req.path, ".css")) type = "text/css; charset=utf-8";
        else if (endsWith(req.path, ".js"))  type = "text/javascript; charset=utf-8";
        else return text(404, "Not found");                  // only web files are ever served

        std::ifstream file(root_ + req.path, std::ios::binary);
        if (!file) return text(404, "Not found");
        std::ostringstream content;
        content << file.rdbuf();
        Response r;
        r.contentType = type;
        r.body = content.str();
        return r;
    }

    static std::string format(const Response& r) {
        std::ostringstream o;
        o << "HTTP/1.1 " << r.status << ' ' << reason(r.status) << "\r\n"
          << "Content-Type: " << r.contentType << "\r\n"
          << "Content-Length: " << r.body.size() << "\r\n"
          << "Cache-Control: no-store\r\n"
          << "Connection: close\r\n";
        if (!r.location.empty()) o << "Location: " << r.location << "\r\n";
        o << "\r\n" << r.body;
        return o.str();
    }

    static void sendAll(net::Socket s, const std::string& data) {
        int flags = 0;
#ifdef MSG_NOSIGNAL
        flags = MSG_NOSIGNAL;
#endif
        std::size_t sent = 0;
        while (sent < data.size()) {
            int n = static_cast<int>(::send(s, data.data() + sent, static_cast<int>(data.size() - sent), flags));
            if (n <= 0) return;
            sent += static_cast<std::size_t>(n);
        }
    }

    void serve(net::Socket s) const {
        setTimeout(s);
        Request req;
        const int status = readRequest(s, req);
        if (status == -1) return;
        Response res;
        if (status != 0) {
            res = text(status, "Bad request");
        } else {
            try {
                if (req.path.rfind("/api/", 0) == 0)
                    res = (req.method == "POST") ? api_(req) : text(405, "Use POST for /api/");
                else
                    res = serveStatic(req);
            } catch (const std::exception& e) {
                res = text(500, e.what());
            }
        }
        sendAll(s, format(res));
    }
};
