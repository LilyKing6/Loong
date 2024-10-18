#include "AutoLock.h"

LLock::LLock()
{
#ifdef _WIN32

    InitializeCriticalSection(&m_Section);
#else

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&m_mutex, &attr);
#endif
}

LLock::~LLock()
{
#ifdef _WIN32
    DeleteCriticalSection(&m_Section);
#else
    pthread_mutex_destroy(&m_mutex);
#endif
}

void LLock::Lock()
{
#ifdef _WIN32
    EnterCriticalSection(&m_Section);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void LLock::UnLock()
{
#ifdef _WIN32
    LeaveCriticalSection(&m_Section);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}

static LLock _clock;

AutoLock::AutoLock()
{
    m_pLock = &_clock;
    m_pLock->Lock();
}

AutoLock::AutoLock(LLock& lock)
{
    m_pLock = &lock;
    m_pLock->Lock();
}

AutoLock::~AutoLock()
{
    if (m_pLock)
        m_pLock->UnLock();
}

