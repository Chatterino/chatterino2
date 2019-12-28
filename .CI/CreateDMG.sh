#!/bin/sh

echo "Running MACDEPLOYQT"
/usr/local/opt/qt/bin/macdeployqt chatterino.app -dmg
echo "Creating APP folder"
mkdir app
echo "Running hdiutil attach on the built DMG"
hdiutil attach chatterino.dmg
echo "Copying chatterino.app into the app folder"
cp -r /Volumes/chatterino/chatterino.app app/
echo "Creating DMG with create-dmg"
create-dmg --volname Chatterino2 --volicon ../resources/chatterino.icns --icon-size 50 --app-drop-link 0 0 --format UDBZ chatterino-osx.dmg app/
echo "DONE"
