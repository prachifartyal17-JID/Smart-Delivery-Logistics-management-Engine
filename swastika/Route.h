// Route.h  (Swastika) - doubly linked list of delivery stops
#pragma once
#include <cstddef>
#include <utility>
#include <iostream>
#include <string>
#include <vector>

class Route {
    struct Node {                       // private: outsiders never touch raw nodes
        std::string name;
        Node* prev = nullptr;
        Node* next = nullptr;
        explicit Node(std::string n) : name(std::move(n)) {}
    };

public:
    // Opaque ticket for one stop. Keep it (e.g. in the hash table) to remove that stop in O(1).
    // Only valid while the stop is still in this Route.
    using Handle = Node*;

    Route() = default;
    ~Route() { clear(); }

    // Owns raw nodes, so copying would double-delete (Rule of Three).
    Route(const Route&) = delete;
    Route& operator=(const Route&) = delete;

    Handle add(const std::string& name) {                 // append at the end, O(1)
        Node* node = new Node(name);
        if (tail_ == nullptr) {
            head_ = tail_ = node;
        } else {
            node->prev = tail_;
            tail_->next = node;
            tail_ = node;
        }
        ++count_;
        return node;
    }

    Handle insertAfter(Handle pos, const std::string& name) {   // reschedule a stop, O(1)
        if (pos == nullptr) return nullptr;
        Node* node = new Node(name);
        node->prev = pos;
        node->next = pos->next;
        if (pos->next != nullptr) pos->next->prev = node;
        else                      tail_ = node;
        pos->next = node;
        ++count_;
        return node;
    }

    bool remove(Handle h) {                               // cancel a stop, O(1)
        if (h == nullptr) return false;
        if (h->prev != nullptr) h->prev->next = h->next; else head_ = h->next;
        if (h->next != nullptr) h->next->prev = h->prev; else tail_ = h->prev;
        delete h;
        --count_;
        return true;
    }

    Handle find(const std::string& name) const {          // O(n) search
        for (Node* cur = head_; cur != nullptr; cur = cur->next)
            if (cur->name == name) return cur;
        return nullptr;
    }

    bool removeByName(const std::string& name) { return remove(find(name)); }

    static const std::string& nameOf(Handle h) { return h->name; }

    std::vector<std::string> names() const {              // stop names from start to end
        std::vector<std::string> out;
        for (Node* cur = head_; cur != nullptr; cur = cur->next) out.push_back(cur->name);
        return out;
    }

    std::size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }

    void print(std::ostream& os = std::cout) const {
        if (head_ == nullptr) { os << "Route is empty.\n"; return; }
        os << "Route (Start to End): ";
        for (Node* cur = head_; cur != nullptr; cur = cur->next) {
            os << cur->name;
            if (cur->next != nullptr) os << " -> ";
        }
        os << '\n';
    }

    void printReverse(std::ostream& os = std::cout) const {
        if (tail_ == nullptr) { os << "Route is empty.\n"; return; }
        os << "Route (Reverse): ";
        for (Node* cur = tail_; cur != nullptr; cur = cur->prev) {
            os << cur->name;
            if (cur->prev != nullptr) os << " -> ";
        }
        os << '\n';
    }

    void clear() {
        Node* cur = head_;
        while (cur != nullptr) {
            Node* next = cur->next;
            delete cur;
            cur = next;
        }
        head_ = tail_ = nullptr;
        count_ = 0;
    }

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t count_ = 0;
};
