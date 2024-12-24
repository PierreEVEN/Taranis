#pragma once

#include <atomic>
#include <thread>

namespace Eng
{

class Spinlock
{
public:
    void lock()
    {
        unsigned my = in.fetch_add(1, std::memory_order_relaxed);
        for (int spin_count = 0; out.load(std::memory_order_acquire) != my || ack_cnt.load(std::memory_order::acquire) != 0; ++spin_count)
        {
            if (spin_count < 16)
                _mm_pause();
            else
            {
                std::this_thread::yield();
                spin_count = 0;
            }
        }
    }

    void unlock()
    {
        out.store(out.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }

    void lock_shared()
    {
        unsigned my = in.fetch_add(1, std::memory_order_relaxed);
        for (int spin_count = 0; out.load(std::memory_order_acquire) != my; ++spin_count)
        {
            if (spin_count < 16)
                _mm_pause();
            else
            {
                std::this_thread::yield();
                spin_count = 0;
            }
        }

        ack_cnt.fetch_add(1, std::memory_order::release);

        out.store(out.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }

    void unlock_shared()
    {
        ack_cnt.fetch_sub(1, std::memory_order::seq_cst);
    }

private:
    std::atomic_uint32_t ack_cnt = 0;
    std::atomic_flag     lk;

    std::atomic<unsigned> in{0};
    std::atomic<unsigned> out{0};
};
} // namespace Eng