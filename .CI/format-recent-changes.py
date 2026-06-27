from datetime import datetime, timezone
import os
import subprocess

def run_git_command(args: list[str]) -> str:
    p = subprocess.run(
        ["git", *args],
        cwd=os.path.dirname(os.path.realpath(__file__)),
        text=True,
        check=True,
        capture_output=True,
    )
    return p.stdout.strip()

def get_last_version_tag() -> str | None:
    try:
        return run_git_command([
            "describe",
            "--tags",
            "--abbrev=0",
            "--match",
            "v*"
        ])
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
        "--pretty=format:%cI|%an|%s",
        "--no-merges",
    ]
    if limit:
        args.insert(1, f"-n{limit}")
    log_output = run_git_command(args)
    unreleased: list[tuple[datetime, str]] = []
    for line in log_output.splitlines():
        if not line.strip():
            continue
        date_str, author, subject = line.split("|", 2)
        if author.lower() == "dependabot[bot]":
            continue
        d = datetime.fromisoformat(date_str).astimezone(timezone.utc)
        content = f"- [{d.strftime('%Y-%m-%d')}] {subject}"
        unreleased.append((d, content))
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
