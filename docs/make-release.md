# Checklist for making a stable release (e.g. 2.5.4)

I will be using `2.3.4` as the example release in this document.

## Before making the release

- [ ] Are there any issues or PRs still labeled to be included in this release? If yes, remove the label/milestone.
- [ ] Do the scopes in `src/providers/twitch/TwitchAccountManager.cpp` match the ones in the website repo? https://github.com/Chatterino/website/blob/main/pages/client_login.tsx

## In the release PR

- [ ] Updated version code in `src/common/Version.hpp`

  ```diff
  - inline const QString CHATTERINO_VERSION = QStringLiteral("2.3.4-beta.1");
  + inline const QString CHATTERINO_VERSION = QStringLiteral("2.3.4");
  ```

- [ ] Updated version code in `CMakeLists.txt`  
       If you made a beta release, this step will not be necessary.

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
  +   <release version="2.3.4" date="2020-02-03">
  +       <url>https://github.com/Chatterino/chatterino2/releases/tag/v2.3.4</url>
  +   </release>
        <release version="2.3.4~beta1" date="2020-02-02">
            <url>https://github.com/Chatterino/chatterino2/releases/tag/v2.3.4-beta.1</url>
        </release>
  ```

- [ ] Updated version code in `.CI/chatterino-installer.iss`  
       If you made a beta release, this step will not be necessary.

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
  + ## 2.3.4

   - Bugfix: Foo (#2)

  ```

## After the PR has been created

You will need to add the `skip-changelog-checker` label to the PR since we are doing something you're not meant to do in a normal PR.

- [ ] Ensure all GitHub API credentials from the `chatterino-ci` user are still valid  
       Sign into the `chatterino-ci` user and validate that the `WinGet` and `Homebrew` Personal access tokens are valid: https://github.com/settings/tokens
- [ ] Make a new tag on `pajlada/serialize`, `pajlada/signals`, and `pajlada/settings` called `chatterino/2.3.4` pointing at the commit hash Chatterino uses.

## After the PR has been merged

- [ ] Tag the release  
       Ensure you're on the correct release locally
  ```sh
  git tag v2.3.4 --annotate --message v2.3.4
  git push origin v2.3.4
  ```
- [ ] Manually run the [create-installer](https://github.com/Chatterino/chatterino2/actions/workflows/create-installer.yml) workflow.  
       This is only necessary if the tag was created after the CI in the main branch finished.
- [ ] If the winget releaser action doesn't work as expected, you can run this manually using [Komac](https://github.com/russellbanks/Komac), replacing `v2.5.2` with the current release:  
       `komac update ChatterinoTeam.Chatterino --version 2.5.2 --urls https://github.com/Chatterino/chatterino2/releases/download/v2.5.2/Chatterino.Installer.exe`
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
  xcrun notarytool submit Chatterino-10.15.dmg --wait --keychain-profile notarytool-password --keychain /Users/pajlada/Library/Keychains/chatterino-2025.keychain-db
  ...
  xcrun notarytool submit Chatterino.dmg --wait --keychain-profile notarytool-password --keychain /Users/pajlada/Library/Keychains/chatterino-2025.keychain-db
  ...
  ```

### Creating the release

- [ ] Create a GitHub release & upload all files in your release directory
- [ ] Link the release to fourtf and ask him to start the release process from his end

## After the binaries have been uploaded to fourtf's bucket

- [ ] Re-run the Publish Homebrew Cask on Release action
- [ ] Update links in the Chatterino website to point to the new release
- [ ] Remove the "hold for release" label on all issues and PRs.
