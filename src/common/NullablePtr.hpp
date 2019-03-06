#pragma once

#include <type_traits>

namespace chatterino
{
    template <typename T>
    class NullablePtr
    {
    public:
        NullablePtr()
            : element_(nullptr)
        {
        }

        NullablePtr(T* element)
            : element_(element)
        {
        }

        T* operator->() const
        {
            assert(this->hasElement());

            return element_;
        }

        typename std::add_lvalue_reference<T>::type operator*() const
        {
            assert(this->hasElement());

            return *element_;
        }

        T* get() const
        {
            assert(this->hasElement());

            return this->element_;
        }

        bool isNull() const
        {
            return this->element_ == nullptr;
        }

        bool hasElement() const
        {
            return this->element_ != nullptr;
        }

        operator bool() const
        {
            return this->hasElement();
        }

        bool operator!() const
        {
            return !this->hasElement();
        }

        template <typename X = T,
            typename = std::enable_if_t<!std::is_const<X>::value>>
        operator NullablePtr<const T>() const
        {
            return NullablePtr<const T>(this->element_);
        }

    private:
        T* element_;
    };

}  // namespace chatterino
