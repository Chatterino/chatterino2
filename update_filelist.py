#!/usr/bin/python3

import os
import re
import subprocess

dir_path = os.path.dirname(os.path.realpath(__file__))
filename = 'chatterino.pro'
data = ""

with open(filename, 'r') as project:
    data = project.read()
    sources_list = subprocess.getoutput("find ./src -type f -regex '.*\.cpp' | sed 's_\./_    _g'").splitlines()
    sources_list.sort(key=str.lower)
    sources = "\n".join(sources_list)
    sources = re.sub(r'$', r' \\\\', sources, flags=re.MULTILINE)
    sources += "\n"
    data = re.sub(r'^SOURCES(.|\r|\n)*?^$', 'SOURCES += \\\n' + sources, data, flags=re.MULTILINE)

    headers_list = subprocess.getoutput("find ./src -type f -regex '.*\.hpp' | sed 's_\./_    _g'").splitlines()
    headers_list.sort(key=str.lower)
    headers = "\n".join(headers_list)
    headers = re.sub(r'$', r' \\\\', headers, flags=re.MULTILINE)
    headers += "\n"
    data = re.sub(r'^HEADERS(.|\r|\n)*?^$', 'HEADERS += \\\n' + headers, data, flags=re.MULTILINE)

with open(filename, 'w') as project:
    project.write(data)

