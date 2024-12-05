// Copyright (c) 2024 Clarisma / GeoDesk contributors
// SPDX-License-Identifier: LGPL-3.0-only

#pragma once
#include <clarisma/thread/Threads.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace clarisma {

namespace Threads
{
    void kill(std::thread& thread)
    {
        #ifdef _WIN32
        HANDLE hThread = thread.native_handle();
        TerminateThread(hThread, 0);
        #else
        pthread_t nativeThread = thread.native_handle();
        pthread_cancel(nativeThread);
        #endif
    }
}
} // namespace clarisma
