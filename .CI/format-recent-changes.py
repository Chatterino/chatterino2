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

def get_last_version_tag() -> str | None:
    try:
        p = subprocess.run(
            ["git", "describe", "--tags", "--abbrev=0", "--match", "v*"],
            cwd=os.path.dirname(os.path.realpath(__file__)),
            text=True,
            check=True,
            capture_output=True,
        )
        return p.stdout.strip()
    except subprocess.CalledProcessError:
        return None

def get_unreleased_commits():
    last_tag = get_last_version_tag()
    if last_tag:
        log_range = f"{last_tag}..HEAD"
        limit = None
    else:
        # no version tag -> just take a few recent commits
        log_range = "HEAD"
        limit = 10
    args = [
        "log",
        log_range,
        "--pretty=format:%cI|%s",
        "--no-merges",
    ]
    if limit:
        args.insert(1, f"-n{limit}")
    try:
        p = subprocess.run(
            ["git", *args],
            cwd=os.path.dirname(os.path.realpath(__file__)),
            text=True,
            check=True,
            capture_output=True,
        )
        log_output = p.stdout.strip()
    except subprocess.CalledProcessError:
        log_output = None
    unreleased: list[tuple[datetime, str]] = []
    for line in log_output.splitlines():
        if not line.strip():
            continue
        date_str, subject = line.split("|", 1)
        d = datetime.fromisoformat(date_str).astimezone(timezone.utc)
        content = f"- [{d.strftime('%Y-%m-%d')}] {subject}"
        unreleased.sort(key=lambda it: it[0], reverse=True)
        return unreleased

unreleased_lines = get_unreleased_commits()

if len(unreleased_lines) == 0:
    print("No changes since last release.")

for _, line in unreleased_lines[:5]:
    print(line)

if len(unreleased_lines) > 5:
    print("<details><summary>More Changes</summary>\n")
    for _, line in unreleased_lines[5:]:
        print(line)
    print("</details>")
