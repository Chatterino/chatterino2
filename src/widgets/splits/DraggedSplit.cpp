#include "widgets/splits/DraggedSplit.hpp"

namespace chatterino {

static bool currentlyDraggingSplit = false;

bool isDraggingSplit()
{
    return currentlyDraggingSplit;
}

void setDraggingSplit(bool isDraggingSplit)
{
    currentlyDraggingSplit = isDraggingSplit;
}

}  // namespace chatterino
