#pragma once

#include "jsonc_utils.hpp"

class interrupt_t
{
private:
    std::string type_;
    json_object* args_;

public:
    explicit interrupt_t() = default;
    explicit interrupt_t(const interrupt_t&) = default;
    explicit interrupt_t(interrupt_t&&) = default;
    ~interrupt_t() = default;

    interrupt_t& operator=(const interrupt_t&) = default;
    interrupt_t& operator=(interrupt_t&&) = default;

    explicit interrupt_t(json_object* o);
    interrupt_t& operator<<(json_object* o);

    std::string type() const;
    json_object* args() const;

    void type(std::string v);
    void args(json_object* v);
};
