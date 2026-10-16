// MinHeap.h  (Prachi) - priority queue. Urgent (low number) first; ties broken by arrival order.
#pragma once
#include <cstddef>
#include <utility>
#include "Parcel.h"

class MinHeap {
public:
    bool isEmpty() const { return size_ == 0; }
    bool isFull() const  { return size_ == MAX_SIZE; }
    std::size_t size() const { return size_; }

    bool insert(Parcel* parcel) {
        if (parcel == nullptr || isFull()) return false;
        heap_[size_] = parcel;
        siftUp(size_);
        ++size_;
        return true;
    }

    Parcel* peek() const { return isEmpty() ? nullptr : heap_[0]; }

    Parcel* remove() {
        if (isEmpty()) return nullptr;
        Parcel* result = heap_[0];
        --size_;
        heap_[0] = heap_[size_];
        siftDown(0);
        return result;
    }

private:
    static constexpr std::size_t MAX_SIZE = 64;
    Parcel* heap_[MAX_SIZE] = {};
    std::size_t size_ = 0;

    // true if 'a' must leave before 'b'
    static bool leavesBefore(const Parcel* a, const Parcel* b) {
        if (a->priority() != b->priority()) return a->priority() < b->priority();
        return a->sequence() < b->sequence();   // FIFO among equal priorities
    }

    void siftUp(std::size_t i) {
        // 'i' is the new slot; size_ is incremented by the caller afterwards
        while (i > 0) {
            std::size_t parent = (i - 1) / 2;
            if (!leavesBefore(heap_[i], heap_[parent])) break;
            std::swap(heap_[i], heap_[parent]);
            i = parent;
        }
    }

    void siftDown(std::size_t i) {
        while (true) {
            std::size_t left = 2 * i + 1;
            if (left >= size_) break;
            std::size_t best = left;
            std::size_t right = left + 1;
            if (right < size_ && leavesBefore(heap_[right], heap_[left])) best = right;
            if (!leavesBefore(heap_[best], heap_[i])) break;
            std::swap(heap_[i], heap_[best]);
            i = best;
        }
    }
};
