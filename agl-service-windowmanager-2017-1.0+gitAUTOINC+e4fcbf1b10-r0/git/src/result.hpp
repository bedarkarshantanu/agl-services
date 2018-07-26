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

#ifndef TMCAGLWM_RESULT_HPP
#define TMCAGLWM_RESULT_HPP

#include <experimental/optional>
#include <functional>

namespace wm
{

using std::experimental::nullopt;
using std::experimental::optional;

// We only ever return a string as an error - so just parametrize
// this over result type T
template <typename T>
struct result
{
  char const *e;
  optional<T> t;

  bool is_ok() const { return this->t != nullopt; }
  bool is_err() const { return this->e != nullptr; }

  T unwrap()
  {
    if (this->e != nullptr)
    {
      throw std::logic_error(this->e);
    }
    return this->t.value();
  }

  operator T() { return this->unwrap(); }

  char const *unwrap_err() { return this->e; }

  optional<T> const &ok() const { return this->t; }
  optional<char const *> err() const
  {
    return this->e ? optional<char const *>(this->e) : nullopt;
  }

  result<T> map_err(std::function<char const *(char const *)> f);
};

template <typename T>
struct result<T> Err(char const *e)
{
  return result<T>{e, nullopt};
}

template <typename T>
struct result<T> Ok(T t)
{
  return result<T>{nullptr, t};
}

template <typename T>
result<T> result<T>::map_err(std::function<char const *(char const *)> f)
{
  if (this->is_err())
  {
    return Err<T>(f(this->e));
  }
  return *this;
}

} // namespace wm

#endif // TMCAGLWM_RESULT_HPP
