from typing import List, Tuple

import logging
import re

CommentCommands = List[Tuple[str, str]]

log = logging.getLogger(__name__)


def parse_comment_commands(raw_comment: str) -> CommentCommands:
    comment_commands: CommentCommands = []

    def clean_comment_line(line: str) -> str:
        return line.replace("/", "").replace("*", "").strip()

    comment_lines = [line for line in map(clean_comment_line, raw_comment.splitlines()) if line != ""]

    for comment in comment_lines:
        parts = comment.split("=", 2)
        if len(parts) != 2:
            continue

        command = parts[0].strip()
        value = parts[1].strip()
        comment_commands.append((command, value))

    return comment_commands


def json_transform(input_str: str, transformation: str) -> str:
    match transformation:
        case "snake_case":
            return re.sub(r"(?<![A-Z])\B[A-Z]", r"_\g<0>", input_str).lower()
        case other:
            log.warning(f"Unknown transformation '{other}', ignoring")
            return input_str
