//
// Created by joseph larralde on 17/03/2026.
//

#ifndef COMPOSESIREN_ATOMICUTILITIES_H
#define COMPOSESIREN_ATOMICUTILITIES_H

#include <atomic>

// ScopedGuard /////////////////////////////////////////////////////////////////

struct ScopedGuard {
    ScopedGuard(std::atomic<bool>& f) : flag(f) {
        flag.store(true, std::memory_order_relaxed);
    }

    ~ScopedGuard() {
        flag.store(false, std::memory_order_relaxed);
    }

    std::atomic<bool>& flag;
};


// MailBox /////////////////////////////////////////////////////////////////////

template<typename T>
class LatestValueMailbox {
public:
    void push(T v) noexcept {
        value.store(v, std::memory_order_relaxed);
        hasValue.store(true, std::memory_order_release);
    }

    bool pop(T& out) noexcept {
        if (!hasValue.load(std::memory_order_acquire))
            return false;

        out = value.load(std::memory_order_relaxed);
        hasValue.store(false, std::memory_order_relaxed);
        return true;
    }

private:
    std::atomic<T> value { };
    std::atomic<bool> hasValue { false };
};


#endif //COMPOSESIREN_ATOMICUTILITIES_H