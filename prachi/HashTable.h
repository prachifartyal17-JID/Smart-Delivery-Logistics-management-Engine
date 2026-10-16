// HashTable.h  (Prachi) - chained hash table: Tracking ID -> Parcel*  (average O(1))
#pragma once
#include <cstddef>
#include <string>
#include "Parcel.h"

class HashTable {
public:
    HashTable() = default;
    ~HashTable() { clear(); }

    // Owns raw nodes, so copying would double-delete (Rule of Three).
    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    void insert(const std::string& id, Parcel* parcel) {
        std::size_t index = hashFunction(id);
        for (BucketNode* cur = table_[index]; cur != nullptr; cur = cur->next) {
            if (cur->key == id) { cur->parcel = parcel; return; }   // update existing
        }
        BucketNode* node = new BucketNode{id, parcel, table_[index]};
        table_[index] = node;
        ++count_;
    }

    Parcel* search(const std::string& id) const {
        for (BucketNode* cur = table_[hashFunction(id)]; cur != nullptr; cur = cur->next) {
            if (cur->key == id) return cur->parcel;
        }
        return nullptr;
    }

    bool remove(const std::string& id) {
        std::size_t index = hashFunction(id);
        BucketNode* prev = nullptr;
        for (BucketNode* cur = table_[index]; cur != nullptr; prev = cur, cur = cur->next) {
            if (cur->key == id) {
                if (prev == nullptr) table_[index] = cur->next;
                else                 prev->next = cur->next;
                delete cur;
                --count_;
                return true;
            }
        }
        return false;
    }

    std::size_t size() const { return count_; }

    void clear() {
        for (std::size_t i = 0; i < TABLE_SIZE; ++i) {
            BucketNode* cur = table_[i];
            while (cur != nullptr) {
                BucketNode* next = cur->next;
                delete cur;
                cur = next;
            }
            table_[i] = nullptr;
        }
        count_ = 0;
    }

private:
    struct BucketNode {
        std::string key;
        Parcel* parcel;
        BucketNode* next;
    };

    static constexpr std::size_t TABLE_SIZE = 101;
    BucketNode* table_[TABLE_SIZE] = {};
    std::size_t count_ = 0;

    // djb2 hash; works for any ID format ("TRK-101" or a 12-digit number as text)
    static std::size_t hashFunction(const std::string& key) {
        std::size_t hash = 5381;
        for (char c : key) hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
        return hash % TABLE_SIZE;
    }
};
