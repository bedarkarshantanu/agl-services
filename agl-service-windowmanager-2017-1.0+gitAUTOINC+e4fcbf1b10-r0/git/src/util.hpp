/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WM_UTIL_HPP
#define WM_UTIL_HPP

#include <functional>
#include <thread>
#include <vector>

#include <sys/poll.h>

#ifndef DO_NOT_USE_AFB
extern "C"
{
#include <afb/afb-binding.h>
};
#endif

#define CONCAT_(X, Y) X##Y
#define CONCAT(X, Y) CONCAT_(X, Y)

#ifdef __GNUC__
#define ATTR_FORMAT(stringindex, firsttocheck) \
    __attribute__((format(printf, stringindex, firsttocheck)))
#define ATTR_NORETURN __attribute__((noreturn))
#else
#define ATTR_FORMAT(stringindex, firsttocheck)
#define ATTR_NORETURN
#endif

#ifdef AFB_BINDING_VERSION
#define lognotice(...) AFB_NOTICE(__VA_ARGS__)
#define logerror(...) AFB_ERROR(__VA_ARGS__)
#define fatal(...)              \
    do                          \
    {                           \
        AFB_ERROR(__VA_ARGS__); \
        abort();                \
    } while (0)
#else
#define lognotice(...)
#define logerror(...)
#define fatal(...) \
    do             \
    {              \
        abort();   \
    } while (0)
#endif

#ifdef DEBUG_OUTPUT
#ifdef AFB_BINDING_VERSION
#define logdebug(...) AFB_DEBUG(__VA_ARGS__)
#else
#define logdebug(...)
#endif
#else
#define logdebug(...)
#endif

#ifndef SCOPE_TRACING
#define ST()
#define STN(N)
#else
#define ST() \
    ScopeTrace __attribute__((unused)) CONCAT(trace_scope_, __LINE__)(__func__)
#define STN(N) \
    ScopeTrace __attribute__((unused)) CONCAT(named_trace_scope_, __LINE__)(#N)

struct ScopeTrace
{
    thread_local static int indent;
    char const *f{};
    explicit ScopeTrace(char const *func);
    ~ScopeTrace();
};
#endif

/**
 * @struct unique_fd
 */
struct unique_fd
{
    int fd{-1};
    unique_fd() = default;
    explicit unique_fd(int f) : fd{f} {}
    operator int() const { return fd; }
    ~unique_fd();
    unique_fd(unique_fd const &) = delete;
    unique_fd &operator=(unique_fd const &) = delete;
    unique_fd(unique_fd &&o) : fd(o.fd) { o.fd = -1; }
    unique_fd &operator=(unique_fd &&o)
    {
        std::swap(this->fd, o.fd);
        return *this;
    }
};

#endif // !WM_UTIL_HPP
