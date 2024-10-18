#pragma once

#ifdef _WIN32
    #include <windows.h>
#else
    #include <pthread.h>
#endif

class LLock
{
public:
    LLock();
    ~LLock();

    void Lock();
    void UnLock();

private:
    
#ifdef _WIN32
    CRITICAL_SECTION m_Section; 
#else
    pthread_mutex_t m_mutex; 
#endif
};


class AutoLock
{
public:
    AutoLock();
    AutoLock(LLock& lock);
    ~AutoLock();

private:
    LLock* m_pLock; 
};
