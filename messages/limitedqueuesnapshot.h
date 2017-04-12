#ifndef LIMITEDQUEUESNAPSHOT_H
#define LIMITEDQUEUESNAPSHOT_H

#include <memory>
#include <vector>

namespace chatterino {
namespace messages {

template <typename T>
class LimitedQueueSnapshot
{
public:
    LimitedQueueSnapshot(std::shared_ptr<std::vector<T>> ptr, int offset, int length)
        : vector(ptr)
        , offset(offset)
        , length(length)
    {
    }

    int getLength()
    {
        return this->length;
    }

    T const &operator[](int index) const
    {
        return this->vector->at(index + this->offset);
    }

private:
    std::shared_ptr<std::vector<T>> vector;

    int offset;
    int length;
};
}
}

#endif  // LIMITEDQUEUESNAPSHOT_H
