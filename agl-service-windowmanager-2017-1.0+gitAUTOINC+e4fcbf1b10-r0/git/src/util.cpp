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

#include "util.hpp"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <unistd.h>

#ifdef SCOPE_TRACING
thread_local int ScopeTrace::indent = 0;
ScopeTrace::ScopeTrace(char const *func) : f(func)
{
    fprintf(stderr, "%lu %*s%s -->\n", pthread_self(), 2 * indent++, "", this->f);
}
ScopeTrace::~ScopeTrace() { fprintf(stderr, "%lu %*s%s <--\n", pthread_self(), 2 * --indent, "", this->f); }
#endif

unique_fd::~unique_fd()
{
    if (this->fd != -1)
    {
        close(this->fd);
    }
}
