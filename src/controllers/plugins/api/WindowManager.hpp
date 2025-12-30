#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <sol/forward.hpp>

/**
 * @includefile widgets/splits/SplitContainer.hpp
 * @includefile widgets/Window.hpp
 */

/* @lua-fragment

---@class c2.Split
---@field channel c2.Channel The channel open in this split (might be empty)
c2.Split = {}

---@class c2.SplitContainerNode A node in a split container
---@field type c2.SplitContainerNodeType The type of this node
---@field split c2.Split|nil The split contained in this code (if this is a split node)
---@field parent c2.SplitContainerNode|nil The parent node
---@field horizontal_flex number The amount of horizontal space this split takes
---@field vertical_flex number The amount of vertical space this split takes
c2.SplitContainerNode = {}

---Get all children of this node.
---@return c2.SplitContainerNode[] children
function c2.SplitContainerNode:children() end

---Is this handle still valid?
---@return boolean
function c2.SplitContainerNode:is_valid() end

---@class c2.SplitContainer A container with potentially multiple splits
---@field selected_split c2.Split The currently selected split.
---@field base_node c2.SplitContainerNode The top level node.
c2.SplitContainer = {}

---Get all splits contained in this container
---@return c2.Split[] splits
function c2.SplitContainer:splits() end

---@class c2.SplitNotebook
---@field selected_page c2.SplitContainer|nil The currently selected page.
---@field page_count integer The number of pages/tabs.
c2.SplitNotebook = {}

---Get the notebook page at a specific index.
---@param i integer The zero based index of the page.
---@return c2.SplitContainer|nil page The page contained at the specified index (zero based).
function c2.SplitNotebook:page_at(i) end

---@class c2.Window
---@field notebook c2.SplitNotebook The notebook of this window.
---@field type c2.WindowType The type of this window.
c2.Window = {}

---@class c2.WindowManager
---@field main_window c2.Window The main window.
---@field last_selected_window c2.Window The last selected window (or the main window if none were selected last).
c2.WindowManager = {}

---Get all open windows.
---@return c2.Window[] windows
function c2.WindowManager:all() end

---@type c2.WindowManager
c2.windows = ...
*/

namespace chatterino::lua::api::windowmanager {

void createUserTypes(sol::table &c2);

}  // namespace chatterino::lua::api::windowmanager

#endif
