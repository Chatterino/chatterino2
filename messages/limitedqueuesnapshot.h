#ifndef LIMITEDQUEUESNAPSHOT_H
#define LIMITEDQUEUESNAPSHOT_H

#include <cassert>
#include <memory>
#include <vector>

namespace chatterino {
namespace messages {

template <typename T>
class LimitedQueueSnapshot
{
public:
    LimitedQueueSnapshot(std::shared_ptr<std::vector<T>> ptr, int offset,
                         int length)
        : vectorPtr(ptr)
        , vector(ptr.get())
        , offset(offset)
        , length(length)
    {
    }

    int
    getLength()
    {
        return length;
    }

    T const &operator[](int index) const
    {
        assert(index >= 0);
        assert(index < length);

        return vector->at(index + offset);
    }

private:
    std::shared_ptr<std::vector<T>> vectorPtr;
    std::vector<T> *vector;

    int offset;
    int length;
};
}
}

#endif  // LIMITEDQUEUESNAPSHOT_H
