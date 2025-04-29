#pragma once
#include "hash.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

class SmallString final
{
    friend struct std::hash<SmallString>;

public:
    SmallString() = default;

    ~SmallString()
    {
        clear();
    }

    SmallString(const char* str)
    {
        init_from_str(str);
    }

    SmallString(const SmallString& o) : SmallString(o.c_str())
    {
    }

    SmallString(SmallString&& o) noexcept : value(o.value)
    {
        o.value = nullptr;
    }

    SmallString& operator=(const SmallString& o)
    {
        if (&o == this)
            return *this;
        clear();
        init_from_str(o.value);
        return *this;
    }

    SmallString& operator=(SmallString&& o) noexcept
    {
        clear();
        init_from_str(o.value);
        o.clear();
        return *this;
    }

    bool empty() const
    {
        return value == nullptr;
    }

    bool operator==(const SmallString& o) const
    {
        if (value == nullptr || o.value == nullptr)
            return value == nullptr && o.value == nullptr;
        size_t i = 0;
        while (value[i] != '\0' && o.value[i] != '\0' && value[i] == o.value[i])
            ++i;
        return value[i] == '\0' && o.value[i] == '\0';
    }

    const char* c_str() const
    {
        return value;
    }

    void clear()
    {
        delete[] value;
        value = nullptr;
    }

private:
    void init_from_str(const char* str)
    {
        if (!str)
            return;
        uint32_t length = 0;
        while (str[length++] != '\0')
        {
        }
        if (length > 1)
        {
            value = new char[length];
            memcpy(value, str, length);
        }
    }

    char* value = nullptr;
};

template <> struct std::hash<SmallString>
{
    size_t operator()(const SmallString& val) const noexcept
    {
        if (val.empty())
            return 0;
        constexpr uint32_t p    = 31;
        constexpr uint32_t m    = static_cast<uint32_t>(1e9) + 9;
        size_t             hash = 0;
        size_t             pow  = 1;
        uint32_t           i    = 0;
        while (val.value[i] != '\0')
        {
            hash = (hash + (val.value[i++] - 'a' + 1) * pow) % m;
            pow  = (pow * p) % m;
        }
        return hash;
    }
};