from typing import Tuple

import logging

log = logging.getLogger(__name__)

definition_markers: Tuple[str, str] = (
    "// DESERIALIZATION DEFINITION START",
    "// DESERIALIZATION DEFINITION END",
)
implementation_markers: Tuple[str, str] = (
    "// DESERIALIZATION IMPLEMENTATION START",
    "// DESERIALIZATION IMPLEMENTATION END",
)


def replace_in_file(
    path: str,
    markers: Tuple[str, str],
    replacement: str,
) -> None:
    with open(path, "r+") as fh:
        # Read file into a list of strings
        lines = [line.rstrip() for line in fh.readlines()]
        start = lines.index(markers[0])
        end = lines.index(markers[1], start)
        log.debug(f"Markers: {start}-{end}")

        lines[start + 1 : end] = [line.rstrip() for line in replacement.splitlines()]

        # Clear file
        fh.truncate(0)
        fh.seek(0)

        # Build new file data
        data = "\n".join(lines)
        # Add a newline at the end of the file
        data += "\n"

        # Write data to file
        fh.write(data)
