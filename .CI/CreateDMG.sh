#!/bin/sh

if [ -d bin/chatterino.app ] && [ ! -d chatterino.app ]; then
    >&2 echo "Moving bin/chatterino.app down one directory"
    mv bin/chatterino.app chatterino.app
fi

if [ ${C2_BUILD_WITH_QT6:-OFF} = ON ]; then
    qtdir=$Qt6_DIR
else
    qtdir=$Qt5_DIR
fi

echo "Running MACDEPLOYQT"
$qtdir/bin/macdeployqt chatterino.app
echo "Creating python3 virtual environment"
python3 -m venv venv
echo "Entering python3 virtual environment"
. venv/bin/activate
echo "Installing dmgbuild"
python3 -m pip install dmgbuild
echo "Running dmgbuild.."
dmgbuild --settings ./../.CI/dmg-settings.py -D app=./chatterino.app Chatterino2 chatterino-osx-Qt-$1.dmg
echo "Done!"
