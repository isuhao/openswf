#pragma once

#include "avm/avm.hpp"

#include <string>

NS_AVM_BEGIN

class GCObject
{
    friend class VirtualMachine;

private:
    uint8_t     m_marked;
    GCObject*   m_next;

public:
    GCObject() : m_marked(0), m_next(nullptr) {}

    uint8_t get_marked_value() const { return m_marked; }
    virtual void mark(uint8_t v);
    virtual std::string to_string() const;
};

NS_AVM_END