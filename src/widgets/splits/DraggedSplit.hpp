#pragma once

namespace chatterino {

// Returns true if the user is currently dragging a split in this Chatterino instance
// We need to keep track of this to ensure splits from other Chatterino instances aren't treated as memory we own
[[nodiscard]] bool isDraggingSplit();

// Set that a split is currently being dragged
// Used by the Split::drag function when a drag is initiated
void startDraggingSplit();

// Set that a split is no longer being dragged
// Used by the Split::drag function when a drag is finished
void stopDraggingSplit();

}  // namespace chatterino
