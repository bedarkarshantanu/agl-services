#include "interrupt.hpp"

interrupt_t::interrupt_t(json_object* o)
{
    jcast(type_, o, "type");
    json_object * value = NULL;
    json_object_object_get_ex(o, "args", &value);
    args_ = value;
}

interrupt_t& interrupt_t::operator<<(json_object* o)
{
    jcast(type_, o, "type");
    json_object * value = NULL;
    json_object_object_get_ex(o, "args", &value);
    args_  = value;
    return *this;
}

std::string interrupt_t::type() const
{
    return type_;
}

json_object* interrupt_t::args() const
{
    return args_;
}

void interrupt_t::type(std::string v)
{
    type_ = v;
}

void interrupt_t::args(json_object* v)
{
    args_ = v;
}
