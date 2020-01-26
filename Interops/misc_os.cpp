#include "misc_os.hpp"
#include <iostream>
#include <cstring>
namespace os_misc {

    Glob::Glob(const char *pattern) {
        std::memset(&glob_result, 0, sizeof(glob_result));
        if (glob(pattern, GLOB_TILDE, NULL, &glob_result) != 0) {
            std::cerr << "glob() error" << std::endl;
            _error = true;
        }
    }

    Glob::~Glob() {
        globfree(&glob_result);
    }

    const char *Glob::operator[](int i) const {
        return glob_result.gl_pathv[i];
    }

    size_t Glob::size() const {
        return _error ? 0 : glob_result.gl_pathc;
    }


    ProcessPool::ProcessPool(size_t nproc) : nproc(nproc) {

    }

    int ProcessPool::fork() {
        if (nproc<=1) // sequential execution
            return 0;
        for (int i = 0; i < nproc; ++i) {
            pid_t child = ::fork();
            if (child < 0) {
//                    std::cerr<<"fork() error"<<std::endl;
                return -2;
            } else if (child == 0) return i;
        }

        while (waitpid(-1, nullptr, 0) > 0);
        return -1;
    }

    ScopedProcess::ScopedProcess(int tid, size_t nproc) : tid(tid), nproc(nproc) {
        this->pid = ::getpid();
    }

    ScopedProcess::~ScopedProcess() {
        if (this->isChild() && nproc>1) // destroy the process itself
            ::_exit(0);
    }

    IPCMutex::IPCMutex() {
        pthread_mutexattr_t attr;
        _err = pthread_mutexattr_init(&attr); if (_err) return;
        _err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED); if (_err) return;
        _err = pthread_mutex_init(&_mutex, &attr); if (_err) return;
        _err = pthread_mutexattr_destroy(&attr); if (_err) return;
        _init = true;
    }

    IPCMutex::~IPCMutex() {
        if(_init)
            pthread_mutex_destroy(&_mutex);
    }

    IPCSem::IPCSem() {
        ::sem_init(&_sem, 1, 1);
    }

    IPCSem::IPCSem(int pshared, unsigned int value) {
        ::sem_init(&_sem, pshared, value);
    }

    IPCSem::~IPCSem() {
        ::sem_destroy(&_sem);
    }

}