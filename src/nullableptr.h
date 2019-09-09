#pragma once

namespace chatterino {

template <typename T>
class NullablePtr
{
public:
    NullablePtr()
        : element_(nullptr)
    {
    }

    NullablePtr(T *element)
        : element_(element)
    {
    }

    T *operator->() const
    {
        assert(this->hasElement());

        return element_;
    }

    T &operator*() const
    {
        assert(this->hasElement());

        return *element_;
    }

    T *get() const
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

private:
    T *element_;
};

}  // namespace chatterino
