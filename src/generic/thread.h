#pragma once
#include <SDL_log.h>
#include <SDL_mutex.h>
#include <SDL_thread.h>

namespace qd {
namespace thread {

class Mutex {
    SDL_mutex* mpMutex;

public:
    Mutex() {
        mpMutex = SDL_CreateMutex();
        if (!mpMutex) {
            SDL_Log("cannot create mutex");
            return;
        }
    }

    Mutex(const Mutex&) = delete;
    void operator=(const Mutex&) = delete;

    ~Mutex() {
        SDL_DestroyMutex(mpMutex);
    }

    inline void lock() {
        SDL_LockMutex(mpMutex);
    }

    inline void unlock() {
        SDL_UnlockMutex(mpMutex);
    }

    bool tryLock() {
        int r = SDL_TryLockMutex(mpMutex);
        return r == 0;
    }

};  // class Mutex
//////////////////////////////////////////////////////////////////////////


class MutexLock {
    Mutex* mMutex;

public:
    MutexLock(Mutex& mutex) : mMutex(&mutex) {
        mMutex->lock();
    }
    MutexLock(MutexLock&& rh) : mMutex(rh.mMutex) {
        rh.mMutex = nullptr;
    }
    MutexLock() = delete;
    ~MutexLock() {
        if (mMutex)
            mMutex->unlock();
    }
};  // class MutexLock
//////////////////////////////////////////////////////////////////////////


class Event {
    SDL_cond* mpCondition;
    SDL_mutex* mpMutex;
    volatile bool mbState;
    bool mbAutoReset;

public:
    Event(bool auto_reset_event = true);
    ~Event();
    void reset();
    void set();
    void wait();
    bool wait(uint32_t time_out_ms);

};  // class thread::Event
//////////////////////////////////////////////////////////////////////////


inline bool isMainThread() {
    return SDL_ThreadID() == SDL_GetThreadID(nullptr);
}

};  // namespace thread
};  // namespace qd
