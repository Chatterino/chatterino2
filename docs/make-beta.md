# Checklist for making a beta release (e.g. 2.3.4-beta.1)

I will be using `2.3.4-beta.1` as the example release in this document.

## In the release PR

- [ ] Updated version code in `src/common/Version.hpp`

  ```diff
  - inline const QString CHATTERINO_VERSION = QStringLiteral("2.3.3");
  + inline const QString CHATTERINO_VERSION = QStringLiteral("2.3.4-beta.1");
  ```

- [ ] Updated version code in `CMakeLists.txt`  
       This can only be "whole versions", so we have to "round it" to the next stable release.

  ```diff
    project(chatterino
  -     VERSION 2.3.3
  +     VERSION 2.3.4
        DESCRIPTION "Chat client for twitch.tv"
        HOMEPAGE_URL "https://chatterino.com/"
    )
  ```

- [ ] Add a new release at the top of the `releases` key in `resources/com.chatterino.chatterino.appdata.xml`  
       The format for beta releases here differs, you have to use a tilde instead, and omit the period before the beta number.

  ```diff
    <releases>
  +   <release version="2.3.4~beta1" date="2020-02-02">
  +       <url>https://github.com/Chatterino/chatterino2/releases/tag/v2.3.4-beta.1</url>
  +   </release>
        <release version="2.3.3" date="2020-01-01">
            <url>https://github.com/Chatterino/chatterino2/releases/tag/v2.3.3</url>
        </release>
  ```

- [ ] Updated version code in `.CI/chatterino-installer.iss`  
       This can only be "whole versions", so we have to "round it" to the next stable release.

  ```diff
    #define MyAppName "Chatterino"
  - #define MyAppVersion "2.3.3"
  + #define MyAppVersion "2.3.4"
    #define MyAppPublisher "Chatterino Team"
  ```

- [ ] Update the changelog `## Unversioned` section to the new version `CHANGELOG.md`  
       Make sure to leave the `## Unversioned` line unchanged for easier merges

  ```diff
   # Changelog

   ## Unversioned
  +
  + ## 2.3.4-beta.1

   - Bugfix: Foo (#1)

  ```

## After the PR has been merged

- [ ] Tag the release  
       Ensure you're on the correct release locally
  ```sh
  git tag v2.3.4-beta.1 --annotate --message v2.3.4-beta.1
  git push origin v2.3.4-beta.1
  ```
- [ ] Manually run the [create-installer](https://github.com/Chatterino/chatterino2/actions/workflows/create-installer.yml) workflow.  
       This is only necessary if the tag was created after the CI in the main branch finished.
- [ ] Ensure changelog on website is up-to-date

## After all GitHub actions have ran

### Prepare the binaries

- [ ] Make a new directory in your `chatterino-releases` directory
- [ ] Find the Create installer action for the release-tagged commit and download:
  - `Chatterino.Installer.exe`
- [ ] Find the Build action for the release-tagged commit and download:
  - `Chatterino-ubuntu-20.04-*.deb`, renamed to `Chatterino-Ubuntu-20.04.deb`
  - `Chatterino-ubuntu-22.04-*.deb`, renamed to `Chatterino-Ubuntu-22.04.deb`
  - `Chatterino-ubuntu-24.04-*.deb`, renamed to `Chatterino-Ubuntu-24.04.deb`
  - `chatterino-windows-x86-64-*-symbols.pdb.7z`, renamed to `Chatterino-Windows-debug-symbols.pdb.7z`
  - `chatterino-windows-x86-64-*.zip`
- [ ] Massage the portable release:
  - Unzip `chatterino-windows-x86-64-*.zip`
  - Edit the `modes` file to say `portable` and nothing else
  - Copy the `updater.1` directory from an old portable release. Tree structure should look like this:
    ```
    Chatterino2
    ├── chatterino.exe
    ...
    ├── updater.1
    │   ├── ChatterinoUpdater.exe
    │   ├── ICSharpCode.SharpZipLib.dll
    │   └── SharpZipLib_LICENSE.txt
    ...
    ```
  - Zip up the portable release again:
  ```sh
  zip -r Chatterino.Portable.zip Chatterino2
  ```
- [ ] create a SHA256 checksum file:
  ```sh
  sha256sum * > sha256-checksums.txt
  ```
- [ ] Verify release structure
  ```
  .
  ├── Chatterino-10.15.dmg
  ├── Chatterino.dmg
  ├── Chatterino.Installer.exe
  ├── Chatterino.Portable.zip
  ├── Chatterino-Ubuntu-20.04.deb
  ├── Chatterino-Ubuntu-22.04.deb
  ├── Chatterino-Ubuntu-24.04.deb
  ├── Chatterino-Windows-debug-symbols.pdb.7z
  └── sha256-checksums.txt
  ```
- [ ] Notarize the macOS releases
  ```sh
  xcrun notarytool submit Chatterino-10.15.dmg --wait --keychain-profile notarytool-password --keychain / Users/pajlada/Library/Keychains/chatterino-2025.keychain-db
  ...
  xcrun notarytool submit Chatterino.dmg --wait --keychain-profile notarytool-password --keychain / Users/pajlada/Library/Keychains/chatterino-2025.keychain-db
  ...
  ```

### Creating the release

- [ ] Create a GitHub release & upload all files in your release directory
- [ ] Link the release to fourtf and ask him to start the release process from his end
