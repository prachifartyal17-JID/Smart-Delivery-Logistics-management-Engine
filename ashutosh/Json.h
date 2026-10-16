// Json.h  (Ashutosh) - tiny JSON helpers. Reads FLAT objects only, e.g. {"id":"1","weight":2.5}
#pragma once
#include <cctype>
#include <cstdlib>
#include <string>

namespace json {

inline std::string escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   if (static_cast<unsigned char>(c) >= 0x20) out += c;
        }
    }
    return out;
}

inline std::string quote(const std::string& s) { return "\"" + escape(s) + "\""; }

// Position of the value after  "key":  (false if the key is missing)
inline bool valueStart(const std::string& body, const std::string& key, std::size_t& pos) {
    const std::string pattern = "\"" + key + "\"";
    std::size_t k = body.find(pattern);
    if (k == std::string::npos) return false;
    k += pattern.size();
    while (k < body.size() && std::isspace(static_cast<unsigned char>(body[k]))) ++k;
    if (k >= body.size() || body[k] != ':') return false;
    ++k;
    while (k < body.size() && std::isspace(static_cast<unsigned char>(body[k]))) ++k;
    pos = k;
    return k < body.size();
}

inline std::string getString(const std::string& body, const std::string& key) {
    std::size_t p = 0;
    if (!valueStart(body, key, p) || body[p] != '"') return "";
    std::string out;
    for (++p; p < body.size() && body[p] != '"'; ++p) {
        if (body[p] == '\\' && p + 1 < body.size()) {
            ++p;
            out += (body[p] == 'n') ? '\n' : (body[p] == 't') ? '\t' : body[p];
        } else {
            out += body[p];
        }
    }
    return out;
}

inline double getNumber(const std::string& body, const std::string& key, double fallback = 0.0) {
    std::size_t p = 0;
    if (!valueStart(body, key, p)) return fallback;
    char* end = nullptr;
    double v = std::strtod(body.c_str() + p, &end);
    return end == body.c_str() + p ? fallback : v;
}

}  // namespace json
