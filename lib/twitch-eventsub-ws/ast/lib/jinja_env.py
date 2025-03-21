import os

from jinja2 import Environment, FileSystemLoader
from jinja2_workarounds import MultiLineInclude

from .membertype import MemberType

template_paths = os.path.join(os.path.dirname(os.path.realpath(__file__)), "templates")

# print(f"Loading jinja templates from {template_paths}")

env = Environment(
    loader=FileSystemLoader(template_paths),
    extensions=[MultiLineInclude],
)
env.globals["MemberType"] = MemberType
