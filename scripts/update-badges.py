import sys
import json
from pathlib import Path


def trim_version(version):
    base_url = version["image_url_1x"].removesuffix("1")
    assert version["image_url_2x"] == base_url + "2"
    assert version["image_url_4x"] == base_url + "3"
    v = {
        "id": version["id"],
        "title": version["title"],
        "image": base_url,
    }
    if version["click_url"]:
        v["url"] = version["click_url"]
    return v


raw = sys.stdin.read()
assert len(raw) > 0, "Response from Helix' chat/badges/global needs to be piped"
base = json.loads(raw)["data"]
out = {set["set_id"]: [trim_version(v) for v in set["versions"]] for set in base}

with open(
    Path(__file__).parent.parent / "resources" / "twitch-badges.json", mode="w"
) as f:
    f.write(json.dumps(out, separators=(",", ":")))
