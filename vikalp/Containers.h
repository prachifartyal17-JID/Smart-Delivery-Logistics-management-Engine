// Containers.h  (Vikalp) - fixed-size circular queue (conveyor belt) and stack (van)
// Class templates: work for any type (we use Parcel*). No dynamic memory, so no leaks.
#pragma once
#include <cstddef>

template <typename T, std::size_t N>
class CircularQueue {
    static_assert(N > 0, "Capacity must be greater than 0");
public:
    bool isEmpty() const { return count_ == 0; }
    bool isFull() const  { return count_ == N; }
    std::size_t size() const { return count_; }
    static constexpr std::size_t capacity() { return N; }

    bool push(const T& value) {            // false if the belt is full
        if (isFull()) return false;
        data_[rear_] = value;
        rear_ = (rear_ + 1) % N;
        ++count_;
        return true;
    }

    bool pop(T& out) {                     // false if the belt is empty
        if (isEmpty()) return false;
        out = data_[front_];
        front_ = (front_ + 1) % N;
        --count_;
        return true;
    }

    bool front(T& out) const {
        if (isEmpty()) return false;
        out = data_[front_];
        return true;
    }

    T at(std::size_t i) const { return data_[(front_ + i) % N]; }   // i = 0 is the front; needs i < size()

private:
    T data_[N] = {};
    std::size_t front_ = 0;
    std::size_t rear_ = 0;
    std::size_t count_ = 0;
};

template <typename T, std::size_t N>
class Stack {
    static_assert(N > 0, "Capacity must be greater than 0");
public:
    bool isEmpty() const { return count_ == 0; }
    bool isFull() const  { return count_ == N; }
    std::size_t size() const { return count_; }
    static constexpr std::size_t capacity() { return N; }

    bool push(const T& value) {            // false if the van is full
        if (isFull()) return false;
        data_[count_++] = value;
        return true;
    }

    bool pop(T& out) {                     // false if empty
        if (isEmpty()) return false;
        out = data_[--count_];
        return true;
    }

    bool peek(T& out) const {
        if (isEmpty()) return false;
        out = data_[count_ - 1];
        return true;
    }

    T at(std::size_t i) const { return data_[i]; }   // i = 0 is the bottom; needs i < size()

private:
    T data_[N] = {};
    std::size_t count_ = 0;
};
