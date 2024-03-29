"""
This script generates docs/plugin-meta.lua. It accepts no arguments

It assumes comments look like:
/**
 * Thing
 *
 * @lua@param thing boolean
 * @lua@returns boolean
 * @exposed name
 */
- Do not have any useful info on '/**' and '*/' lines.
- Class members are not allowed to have non-@command lines and commands different from @lua@field

When this scripts sees "@brief", any further lines of the comment will be ignored

Valid commands are:
1. @exposeenum [dotted.name.in_lua.last_part]
    Define a table with keys of the enum. Values behind those keys aren't
    written on purpose.
    This generates three lines:
     - An type alias of [last_part] to integer,
     - A type description that describes available values of the enum,
     - A global table definition for the num
2. @lua[@command]
    Writes [@command] to the file as a comment, usually this is @class, @param, @return, ...
    @lua@class and @lua@field have special treatment when it comes to generation of spacing new lines
3. @exposed [c2.name]
    Generates a function definition line from the last `@lua@param`s.

Non-command lines of comments are written with a space after '---'
"""
from pathlib import Path

BOILERPLATE = """
---@meta Chatterino2

-- This file is automatically generated from src/controllers/plugins/LuaAPI.hpp by the scripts/make_luals_meta.py script
-- This file is intended to be used with LuaLS (https://luals.github.io/).
-- Add the folder this file is in to "Lua.workspace.library".

c2 = {}

---@class IWeakResource

--- Returns true if the channel this object points to is valid.
--- If the object expired, returns false
--- If given a non-Channel object, it errors.
---@return boolean
function IWeakResource:is_valid() end

"""

repo_root = Path(__file__).parent.parent
lua_api_file = repo_root / "src" / "controllers" / "plugins" / "LuaAPI.hpp"
lua_meta = repo_root / "docs" / "plugin-meta.lua"

print("Writing to", lua_meta.relative_to(repo_root))


def process_file(target, out):
    print("Reading from", target.relative_to(repo_root))
    with target.open("r") as f:
        lines = f.read().splitlines()

    # Are we in a doc comment?
    comment: bool = False
    # This is set when @brief is encountered, making the rest of the comment be
    # ignored
    ignore_this_comment: bool = False

    # Last `@lua@param`s seen - for @exposed generation
    last_params_names: list[str] = []
    # Are we in a `@lua@class` definition? - makes newlines around @lua@class and @lua@field prettier
    is_class = False

    # The name of the next enum in lua world
    expose_next_enum_as: str | None = None
    # Name of the current enum in c++ world, used to generate internal typenames for
    current_enum_name: str | None = None
    for line_num, line in enumerate(lines):
        line = line.strip()
        loc = f'{target.relative_to(repo_root)}:{line_num}'
        if line.startswith("enum class "):
            line = line.removeprefix("enum class ")
            temp = line.split(" ", 2)
            current_enum_name = temp[0]
            if not expose_next_enum_as:
                print(
                    f"{loc} Skipping enum {current_enum_name}, there wasn't a @exposeenum command"
                )
                current_enum_name = None
                continue
            current_enum_name = expose_next_enum_as.split(".", 1)[-1]
            out.write("---@alias " + current_enum_name + " integer\n")
            out.write("---@type { ")
            # temp[1] is '{'
            if len(temp) == 2:  # no values on this line
                continue
            line = temp[2]

        if current_enum_name is not None:
            for i, tok in enumerate(line.split(" ")):
                if tok == "};":
                    break
                entry = tok.removesuffix(",")
                if i != 0:
                    out.write(", ")
                out.write(entry + ": " + current_enum_name)
            out.write(" }\n" f"{expose_next_enum_as} = {{}}\n")
            print(f"{loc} Wrote enum {expose_next_enum_as} => {current_enum_name}")
            current_enum_name = None
            expose_next_enum_as = None
            continue

        if line.startswith("/**"):
            comment = True
            continue
        elif "*/" in line:
            comment = False
            ignore_this_comment = False

            if not is_class:
                out.write("\n")
            continue
        if not comment:
            continue
        if ignore_this_comment:
            continue
        line = line.replace("*", "", 1).lstrip()
        if line == "":
            out.write("---\n")
        elif line.startswith('@brief '):
            # Doxygen comment, on a C++ only method
            ignore_this_comment = True
        elif line.startswith("@exposeenum "):
            expose_next_enum_as = line.split(" ", 1)[1]
        elif line.startswith("@exposed "):
            exp = line.replace("@exposed ", "", 1)
            params = ", ".join(last_params_names)
            out.write(f"function {exp}({params}) end\n")
            print(f"{loc} Wrote function {exp}(...)")
            last_params_names = []
        elif line.startswith("@includefile "):
            filename = line.replace("@includefile ", "", 1)
            output.write(f"-- Now including data from src/{filename}.\n")
            process_file(repo_root / 'src' / filename, output)
            output.write(f'-- Back to {target.relative_to(repo_root)}.\n')
        elif line.startswith("@lua"):
            command = line.replace("@lua", "", 1)
            if command.startswith("@param"):
                last_params_names.append(command.split(" ", 2)[1])
            elif command.startswith("@class"):
                print(f"{loc} Writing {command}")
                if is_class:
                    out.write("\n")
                is_class = True
            elif not command.startswith("@field"):
                is_class = False

            out.write("---" + command + "\n")
        else:
            if is_class:
                is_class = False
                out.write("\n")

            # note the space difference from the branch above
            out.write("--- " + line + "\n")


with lua_meta.open("w") as output:
    output.write(BOILERPLATE[1:])  # skip the newline after triple quote
    process_file(lua_api_file, output)
