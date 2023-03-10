#!/bin/sh

if [ -d bin/chatterino.app ] && [ ! -d chatterino.app ]; then
    >&2 echo "Moving bin/chatterino.app down one directory"
    mv bin/chatterino.app chatterino.app
fi

echo "Running MACDEPLOYQT"
$Qt5_DIR/bin/macdeployqt chatterino.app
echo "Creating python3 virtual environment"
python3 -m venv venv
echo "Entering python3 virtual environment"
. venv/bin/activate
echo "Installing dmgbuild"
python3 -m pip install dmgbuild
echo "Running dmgbuild.."
dmgbuild --settings ./../.CI/dmg-settings.py -D app=./chatterino.app Chatterino2 chatterino-osx.dmg
echo "Done!"
