#include "widgets/splits/DraggedSplit.hpp"

#include <cassert>

namespace chatterino {

static bool currentlyDraggingSplit = false;

bool isDraggingSplit()
{
    return currentlyDraggingSplit;
}

void startDraggingSplit()
{
    assert(currentlyDraggingSplit == false);

    currentlyDraggingSplit = true;
}

void stopDraggingSplit()
{
    assert(currentlyDraggingSplit == true);

    currentlyDraggingSplit = false;
}

}  // namespace chatterino
