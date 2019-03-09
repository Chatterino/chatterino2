#pragma once

#include <memory>

namespace ab
{
    template <typename T>
    class AnyPtr
    {
        AnyPtr(const std::shared_ptr<T>& shared)
            : shared_(shared)
        {
        }

        AnyPtr(std::shared_ptr<T>&& shared)
            : shared_(std::move(shared))
        {
        }

        AnyPtr(std::unique_ptr<T>&& unique)
            : unique_(std::move(unique))
        {
        }

        AnyPtr(T* nonOwning)
            : nonOwning_(nonOwning)
        {
        }

        AnyPtr(AnyPtr<T>&& other)
        {
            this->operator=(other);
        }

        AnyPtr<T>& operator=(const std::shared_ptr<T>& shared)
        {
            this->shared_ = shared;

            this->unique_ = nullptr;
            this->nonOwning_ = nullptr;
        }

        AnyPtr<T>& operator=(std::shared_ptr<T>&& shared)
        {
            this->shared_ = std::move(shared);

            this->unique_ = nullptr;
            this->nonOwning_ = nullptr;
        }

        AnyPtr<T>& operator=(std::unique_ptr<T>&& unique)
        {
            this->unique_ = std::move(unique);

            this->shared_ = nullptr;
            this->nonOwning_ = nullptr;
        }

        AnyPtr<T>& operator=(T* nonOwning)
        {
            this->nonOwning_ = nonOwning;

            this->shared_ = nullptr;
            this->unique_ = nullptr;
        }

        AnyPtr<T>& operator=(AnyPtr<T>&& other)
        {
            if (other.shared_)
                this->shared_ = std::move(other.shared_);
            else if (other.unique_)
                this->unique_ = std::move(other.unique_);
            else
                this->nonOwning_ = other.nonOwning_;
        }

        operator bool() const
        {
            return this->get();
        }

        T& operator*() const
        {
            assert(this->get());
            return *this->get();
        }

        T* operator->() const
        {
            return this->get();
        }

        T* get() const
        {
            if (this->shared_)
                return this->shared_.get();
            else if (this->unique_)
                return this->unique_.get();
            else
                return this->nonOwning_;
        }

    private:
        std::shared_ptr<T> shared_;
        std::unique_ptr<T> unique_;
        T* nonOwning_;
    };
}  // namespace ab
