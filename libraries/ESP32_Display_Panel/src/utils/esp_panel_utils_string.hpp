/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// #include <iostream>
#include <string>
#include <memory>
#include <stdexcept>
#include "esp_lib_utils.h"

namespace esp_panel::utils {

template <typename Allocator = std::allocator<char>>
class CustomString {
public:
    using string_type = std::basic_string<char, std::char_traits<char>, Allocator>;
    using traits_type = typename string_type::traits_type;
    using allocator_type = Allocator;
    using value_type = typename string_type::value_type;
    using size_type = typename string_type::size_type;
    using difference_type = typename string_type::difference_type;
    using reference = typename string_type::reference;
    using const_reference = typename string_type::const_reference;
    using pointer = typename string_type::pointer;
    using const_pointer = typename string_type::const_pointer;
    using iterator = typename string_type::iterator;
    using const_iterator = typename string_type::const_iterator;

    // Constructors
    CustomString() = default;
    CustomString(const char *str) : str_(str) {}
    CustomString(const string_type &str) : str_(str) {}
    CustomString(string_type &&str) noexcept : str_(std::move(str)) {}

    // Destructor
    ~CustomString() = default;

    // Conversion operations
    operator string_type &()
    {
        return str_;
    }
    operator const string_type &() const
    {
        return str_;
    }

    // Basic interface (forwarding std::string methods)
    const char *c_str() const noexcept
    {
        return str_.c_str();
    }
    size_type size() const noexcept
    {
        return str_.size();
    }
    size_type length() const noexcept
    {
        return str_.length();
    }
    size_type capacity() const noexcept
    {
        return str_.capacity();
    }
    void clear() noexcept
    {
        str_.clear();
    }
    bool empty() const noexcept
    {
        return str_.empty();
    }
    void resize(size_type n)
    {
        str_.resize(n);
    }
    void reserve(size_type n)
    {
        str_.reserve(n);
    }

    // Element access
    reference operator[](size_type pos)
    {
        return str_[pos];
    }
    const_reference operator[](size_type pos) const
    {
        return str_[pos];
    }
    reference at(size_type pos)
    {
        return str_.at(pos);
    }
    const_reference at(size_type pos) const
    {
        return str_.at(pos);
    }
    reference front()
    {
        return str_.front();
    }
    const_reference front() const
    {
        return str_.front();
    }
    reference back()
    {
        return str_.back();
    }
    const_reference back() const
    {
        return str_.back();
    }

    // Modifiers
    CustomString &operator+=(const CustomString &other)
    {
        str_ += other.str_;
        return *this;
    }
    CustomString &operator+=(const string_type &other)
    {
        str_ += other;
        return *this;
    }
    CustomString &operator+=(const char *other)
    {
        str_ += other;
        return *this;
    }
    CustomString &operator+=(char ch)
    {
        str_ += ch;
        return *this;
    }

    // Comparison operators
    bool operator==(const CustomString &other) const
    {
        return str_ == other.str_;
    }

    bool operator!=(const CustomString &other) const
    {
        return !(*this == other);
    }

    CustomString &append(const CustomString &other)
    {
        str_.append(other.str_);
        return *this;
    }
    CustomString &append(const string_type &other)
    {
        str_.append(other);
        return *this;
    }
    CustomString &append(const char *s, size_type n)
    {
        str_.append(s, n);
        return *this;
    }
    CustomString &append(const char *s)
    {
        str_.append(s);
        return *this;
    }

    void push_back(char ch)
    {
        str_.push_back(ch);
    }
    void pop_back()
    {
        str_.pop_back();
    }

    // Search operations
    size_type find(const CustomString &other, size_type pos = 0) const
    {
        return str_.find(other.str_, pos);
    }
    size_type find(const string_type &other, size_type pos = 0) const
    {
        return str_.find(other, pos);
    }
    size_type find(const char *s, size_type pos = 0) const
    {
        return str_.find(s, pos);
    }
    size_type find(char ch, size_type pos = 0) const
    {
        return str_.find(ch, pos);
    }

    // Iterators
    iterator begin() noexcept
    {
        return str_.begin();
    }
    const_iterator begin() const noexcept
    {
        return str_.begin();
    }
    iterator end() noexcept
    {
        return str_.end();
    }
    const_iterator end() const noexcept
    {
        return str_.end();
    }

    // Output operator
    // friend std::ostream& operator<<(std::ostream& os, const CustomString& obj) {
    //     return os << obj.str_;
    // }

private:
    string_type str_;
};

// Overloaded operator+ (supporting various combinations)
template <typename Allocator>
CustomString<Allocator> operator+(const CustomString<Allocator> &lhs, const CustomString<Allocator> &rhs)
{
    return CustomString<Allocator>(lhs) += rhs;
}

template <typename Allocator>
CustomString<Allocator> operator+(const CustomString<Allocator> &lhs, const std::basic_string<char> &rhs)
{
    return CustomString<Allocator>(lhs) += rhs;
}

template <typename Allocator>
CustomString<Allocator> operator+(const std::basic_string<char> &lhs, const CustomString<Allocator> &rhs)
{
    return CustomString<Allocator>(lhs) += rhs;
}

template <typename Allocator>
CustomString<Allocator> operator+(const CustomString<Allocator> &lhs, const char *rhs)
{
    return CustomString<Allocator>(lhs) += rhs;
}

template <typename Allocator>
CustomString<Allocator> operator+(const char *lhs, const CustomString<Allocator> &rhs)
{
    return CustomString<Allocator>(lhs) += rhs;
}

template <typename Allocator>
bool operator==(const CustomString<Allocator> &lhs, const char *rhs)
{
    return lhs.str_ == rhs;
}

template <typename Allocator>
bool operator==(const char *lhs, const CustomString<Allocator> &rhs)
{
    return rhs == lhs;
}

template <typename Allocator>
bool operator==(const CustomString<Allocator> &lhs, const std::basic_string<char> &rhs)
{
    return lhs.str_ == rhs;
}

template <typename Allocator>
bool operator==(const std::basic_string<char> &lhs, const CustomString<Allocator> &rhs)
{
    return rhs == lhs;
}

using string = CustomString<esp_utils::GeneralMemoryAllocator<char>>;

} // namespace esp_panel::utils

namespace std {

template<typename Allocator>
struct hash<esp_panel::utils::CustomString<Allocator>> {
    std::size_t operator()(const esp_panel::utils::CustomString<Allocator> &s) const
    {
        return std::hash<std::string_view> {}(std::string_view(s.c_str(), s.size()));
    }
};

} // namespace std
