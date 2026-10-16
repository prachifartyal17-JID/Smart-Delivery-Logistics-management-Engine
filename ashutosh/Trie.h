// Trie.h  (Ashutosh) - prefix tree for pincode / place-name auto-complete, O(L) per lookup
#pragma once
#include <array>
#include <cctype>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class Trie {
public:
    struct Entry { std::string pin; std::string name; };

    // One place is stored under two keys: its pincode and its name (case-insensitive).
    void insert(const std::string& pin, const std::string& name) {
        entries_.push_back(std::make_unique<Entry>(Entry{pin, name}));
        const Entry* e = entries_.back().get();
        add(pin, e);
        add(name, e);
    }

    // Exact key lookup (nullptr if the key is not a stored pincode or name).
    const Entry* find(const std::string& key) const {
        const Node* n = walk(key);
        return n ? n->entry : nullptr;
    }

    // Everything that starts with 'prefix', pins in ascending order.
    std::vector<Entry> suggest(const std::string& prefix, std::size_t limit = 5) const {
        std::vector<Entry> out;
        if (const Node* n = walk(prefix)) collect(n, out, limit);
        return out;
    }

private:
    struct Node {
        std::array<std::unique_ptr<Node>, 128> next{};
        const Entry* entry = nullptr;
    };

    Node root_;
    std::vector<std::unique_ptr<Entry>> entries_;

    static int slot(char c) {
        int v = std::tolower(static_cast<unsigned char>(c));
        return (v >= 0 && v < 128) ? v : -1;
    }

    void add(const std::string& key, const Entry* e) {
        Node* cur = &root_;
        for (char c : key) {
            int s = slot(c);
            if (s < 0) return;                       // skip keys with non-ASCII characters
            if (!cur->next[s]) cur->next[s] = std::make_unique<Node>();
            cur = cur->next[s].get();
        }
        cur->entry = e;
    }

    const Node* walk(const std::string& key) const {
        const Node* cur = &root_;
        for (char c : key) {
            int s = slot(c);
            if (s < 0) return nullptr;
            cur = cur->next[s].get();
            if (cur == nullptr) return nullptr;
        }
        return cur;
    }

    static void collect(const Node* n, std::vector<Entry>& out, std::size_t limit) {
        if (out.size() >= limit) return;
        if (n->entry) {
            bool seen = false;
            for (const Entry& o : out) if (o.pin == n->entry->pin) seen = true;
            if (!seen) out.push_back(*n->entry);
        }
        for (const auto& child : n->next) if (child) collect(child.get(), out, limit);
    }
};
