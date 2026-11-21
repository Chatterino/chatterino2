// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <boost/beast/core/buffer_traits.hpp>
#include <QByteArray>

namespace chatterino {

/// A ConstBufferSequence over a single QByteArray.
/// https://www.boost.org/doc/libs/1_87_0/doc/html/boost_asio/reference/ConstBufferSequence.html
struct QByteArrayBuffer {
    struct QByteArrayHolder {
        QByteArray data;

        operator boost::asio::const_buffer() const
        {
            return {
                this->data.constData(),
                static_cast<size_t>(this->data.size()),
            };
        }
    };

    struct ConstIterator : public std::bidirectional_iterator_tag {
        // using iterator_category = std::bidirectional_iterator_tag;
        using value_type = QByteArrayHolder;
        using difference_type = ptrdiff_t;
        using pointer = const QByteArrayHolder *;
        using reference = const QByteArrayHolder &;

        constexpr ConstIterator() noexcept = default;

        constexpr explicit ConstIterator(pointer ptr) noexcept
            : ptr(ptr)
        {
        }

        [[nodiscard]] constexpr reference operator*() const noexcept
        {
            return *this->ptr;
        }

        [[nodiscard]] constexpr pointer operator->() const noexcept
        {
            return this->ptr;
        }

        constexpr ConstIterator &operator++() noexcept
        {
            ++this->ptr;
            return *this;
        }

        constexpr ConstIterator operator++(int) noexcept
        {
            ConstIterator tmp = *this;
            ++this->ptr;
            return tmp;
        }

        constexpr ConstIterator &operator--() noexcept
        {
            --this->ptr;
            return *this;
        }

        constexpr ConstIterator operator--(int) noexcept
        {
            ConstIterator tmp = *this;
            --this->ptr;
            return tmp;
        }

        [[nodiscard]] constexpr auto operator==(
            const ConstIterator &rhs) const noexcept
        {
            return this->ptr == rhs.ptr;
        }

        const QByteArrayHolder *ptr = nullptr;
    };

    using value_type = QByteArrayHolder;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = QByteArrayHolder *;
    using const_pointer = const QByteArrayHolder *;
    using reference = QByteArrayHolder &;
    using const_reference = const QByteArrayHolder &;
    using iterator = ConstIterator;
    using const_iterator = ConstIterator;

    QByteArrayBuffer(QByteArray ba)
        : holder({std::move(ba)})
    {
    }

    const_iterator begin() const
    {
        return ConstIterator(&this->holder);
    }

    const_iterator end() const
    {
        return ConstIterator((&this->holder) + 1);
    }

    QByteArray data() const
    {
        return this->holder.data;
    }

    QByteArrayHolder holder;
};

static_assert(sizeof(QByteArrayBuffer) == sizeof(QByteArray));

}  // namespace chatterino

static_assert(boost::beast::is_const_buffer_sequence<
              chatterino::QByteArrayBuffer>::value);
