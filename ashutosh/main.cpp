// main.cpp  (Ashutosh) - starts the hub and the web server.
// Run from the project folder:  ./hub        then open  http://localhost:3000
#include <cstdlib>
#include <iostream>
#include "HttpServer.h"
#include "HubManager.h"
#include "Json.h"

static Response api(HubManager& hub, const Request& req) {
    Response res;
    try {
        res.body = hub.handle(req.path.substr(5), req.body);          // strip "/api/"
    } catch (const UnknownEndpoint& e) {
        res.status = 404;
        res.body = "{\"error\":" + json::quote(e.what()) + "}";
    } catch (const std::exception& e) {
        res.status = 400;
        res.body = "{\"error\":" + json::quote(e.what()) + "}";
    }
    return res;
}

int main(int argc, char** argv) {
    const int port = (argc > 1) ? std::atoi(argv[1]) : 3000;
    try {
        HubManager hub("Central Hub");
        HttpServer server(port, ".", [&hub](const Request& r) { return api(hub, r); });
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
