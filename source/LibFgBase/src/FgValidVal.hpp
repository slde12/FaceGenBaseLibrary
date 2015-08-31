//
// Copyright (c) 2015 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// Authors:     Andrew Beatty
// Created:     May 21, 2009
//
// Type extender that remembers if it's been initialized before use.
//
// USE:
//
// To avoid additional memory use for numerical types, use 'FgValid' but note that
// numeric_limits<T>::max() is the special 'invalid' value.
//
// Otherwise, use 'FgValidVal' which adds a bool to keep track.
//


#ifndef FGVALIDVAL_HPP
#define FGVALIDVAL_HPP

#include "FgStdLibs.hpp"
#include "FgSerialize.hpp"

template<typename T>
struct FgValid
{
    T       m_val;      // = numeric_limits<T>::max() if not valid

    FgValid()
    : m_val(std::numeric_limits<T>::max())
    {}

    explicit
    FgValid(const T & v)
    : m_val(v)
    {}

    FgValid &
    operator=(const T & v)
    {m_val = v; return *this; }

    bool
    valid() const
    {return (m_val != std::numeric_limits<T>::max()); }

    void
    invalidate()
    {m_val = std::numeric_limits<T>::max(); }

    // Implicit conversion caused inexplicable errors with gcc and explicit conversion
    // is required in many cases anyway:
    T
    val() const
    {FGASSERT(valid()); return m_val; }

    // The constPtr() and ptr() functions below couldn't be overloads of operator&() since
    // this doesn't play nice with standard library containers:
    const T *
    constPtr() const
    {FGASSERT(valid()); return &m_val; }

    // You're on your own if you use this one, NO CHECKING, since it may be used to set the val,
    // so do a manual check using valid() if you need one along with non-const pointer access:
    T *
    ptr()
    {return &m_val; }

    FG_SERIALIZE1(m_val)
};

template<typename T>
std::ostream &
operator<<(std::ostream & os,const FgValid<T> & v)
{
    if (v.valid())
        return (os << v.val());
    else
        return (os << "<invalid>");
}

template<typename T>
class   FgValidVal
{
public:
    FgValidVal()
    : m_valid(false)
    {}

    FgValidVal(const T & v)
    : m_valid(true), m_val(v)
    {}

    FgValidVal &
    operator=(const T & v)
    {m_val=v; m_valid=true; return *this; }

    bool
    valid() const
    {return m_valid; }

    void
    invalidate()
    {m_valid = false; }

    T &
    ref()
    {FGASSERT(m_valid); return m_val; }

    const T &
    val() const
    {FGASSERT(m_valid); return m_val; }

    template<typename U>
    FgValidVal<U>
    cast()
    {
        if (m_valid)
            return FgValidVal<U>(U(m_val));
        return FgValidVal<U>();
    }

private:
    bool        m_valid;
    T           m_val;
};

#endif
