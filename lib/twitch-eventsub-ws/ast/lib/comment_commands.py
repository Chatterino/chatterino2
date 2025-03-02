from typing import Optional

import logging
import re

log = logging.getLogger(__name__)


class CommentCommands:
    # Transform the key from whatever-case
    # By default, all keys are transformed into snake_case
    # e.g. `userID` transforms to `user_id`
    name_transform: str = "snake_case"

    # Whether the key should completely change its name
    # If set, `name_transform` will do nothing
    name_change: Optional[str] = None

    # Don't fail when an optional object exists and its data is bad
    dont_fail_on_deserialization: bool = False

    # Deserialization hint, current use-cases can be replaced with
    # https://github.com/Chatterino/chatterino2/issues/5912
    tag: Optional[str] = None

    inner_root: str = ""

    default: Optional[str] = None

    def __init__(self, parent: Optional["CommentCommands"] = None) -> None:
        if parent is not None:
            self.name_transform = parent.name_transform
            self.name_change = parent.name_change
            self.dont_fail_on_deserialization = parent.dont_fail_on_deserialization
            self.tag = parent.tag
            self.inner_root = parent.inner_root

    def parse(self, raw_comment: str) -> None:
        def clean_comment_line(line: str) -> str:
            return line.replace("/", "").replace("*", "").strip()

        comment_lines = [line for line in map(clean_comment_line, raw_comment.splitlines()) if line != ""]

        for comment in comment_lines:
            parts = comment.split("=", 2)
            if len(parts) != 2:
                continue

            command = parts[0].strip()
            value = parts[1].strip()

            match command:
                case "json_rename":
                    self.name_change = value
                case "json_dont_fail_on_deserialization":
                    self.dont_fail_on_deserialization = bool(value.lower() == "true")
                case "json_transform":
                    self.name_transform = value
                case "json_inner":
                    self.inner_root = value
                    pass
                case "json_tag":
                    self.tag = value
                case "default":
                    self.default = value
                case other:
                    log.warning(f"Unknown comment command found: {other} with value {value}")

    def apply_name_transform(self, input_json_name: str) -> str:
        if self.name_change is not None:
            return self.name_change

        match self.name_transform:
            case "snake_case":
                return re.sub(r"(?<![A-Z])\B[A-Z]", r"_\g<0>", input_json_name).lower()

            case other:
                log.warning(f"Unknown transformation '{other}', ignoring")
                return input_json_name


def json_transform(input_str: str, transformation: str) -> str:
    match transformation:
        case "snake_case":
            return re.sub(r"(?<![A-Z])\B[A-Z]", r"_\g<0>", input_str).lower()
        case other:
            log.warning(f"Unknown transformation '{other}', ignoring")
            return input_str
