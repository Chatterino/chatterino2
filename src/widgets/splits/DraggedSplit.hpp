#pragma once

namespace chatterino {

// Returns true if the user is currently dragging a split in this Chatterino instance
// We need to keep track of this to ensure splits from other Chatterino instances aren't treated as memory we own
[[nodiscard]] bool isDraggingSplit();

// Set the current split dragging state
// Controlled by Split's drag function
void setDraggingSplit(bool isDraggingSplit);

}  // namespace chatterino
