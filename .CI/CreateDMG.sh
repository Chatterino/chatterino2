#!/bin/sh

python3 --version

echo "Running MACDEPLOYQT"
/usr/local/opt/qt/bin/macdeployqt chatterino.app
echo "Creating python3 virtual environment"
python3 -m venv venv
echo "Entering python3 virtual environment"
. venv/bin/activate
echo "Installing dmgbuild"
python3 -m pip install dmgbuild
echo "Installing dmgbuild dependencies"
python3 -m pip install biplist
echo "Running dmgbuild.."
dmgbuild --settings ./../.CI/dmg-settings.py -D app=./chatterino.app Chatterino2 chatterino-osx.dmg
echo "Done!"
