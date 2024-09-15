from datetime import datetime, timezone
import os
import subprocess
import re

LINE_REGEX = re.compile(
    r"""(?x)
^(?P<commit>[A-Fa-f0-9]+)\s+
\(
    <(?P<email>[^>]+)>\s+
    (?P<date>[^\s]+\s[^\s]+\s[^\s]+)\s+
    (?P<line>\d+)
\)\s
(?P<content>.*)$
"""
)
VERSION_REGEX = re.compile(r"^#+\s*v?\d")


def get_unreleased_lines(file: str):
    # contains lines in the form of
    # {commit-sha} (<{email}>\s+{date}\s+{line-no}) {line}
    p = subprocess.run(
        ["git", "blame", "-e", "--date=iso", file],
        cwd=os.path.dirname(os.path.realpath(__file__)),
        text=True,
        check=True,
        capture_output=True,
    )

    unreleased_lines: list[tuple[datetime, str]] = []
    for line in p.stdout.splitlines():
        if not line:
            continue
        m = LINE_REGEX.match(line)
        assert m, f"Failed to match '{line}'"
        content = m.group("content")

        if not content:
            continue
        if content.startswith("#"):
            if VERSION_REGEX.match(content):
                break
            continue  # ignore lines with '#'

        d = datetime.fromisoformat(m.group("date"))
        d = d.astimezone(tz=timezone.utc)
        content = content.replace("- ", f"- [{d.strftime('%Y-%m-%d')}] ", 1)
        unreleased_lines.append((d, content))

    unreleased_lines.sort(key=lambda it: it[0], reverse=True)
    return unreleased_lines


unreleased_lines = get_unreleased_lines("../CHANGELOG.md")

if len(unreleased_lines) == 0:
    print("No changes since last release.")

for _, line in unreleased_lines[:5]:
    print(line)

if len(unreleased_lines) > 5:
    print("<details><summary>More Changes</summary>\n")
    for _, line in unreleased_lines[5:]:
        print(line)
    print("</details>")
