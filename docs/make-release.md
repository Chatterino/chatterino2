# Checklist for making a release

## In the release PR

- [ ] Updated version code in `src/common/Version.hpp`
- [ ] Updated version code in `CMakeLists.txt`  
       This can only be "whole versions", so if you're releasing `2.4.0-beta` you'll need to condense it to `2.4.0`
- [ ] Add a new release at the top of the `releases` key in `resources/com.chatterino.chatterino.appdata.xml`  
       This cannot use dash to denote a pre-release identifier, you have to use a tilde instead.

- [ ] Updated version code in `.CI/chatterino-installer.iss`  
       This can only be "whole versions", so if you're releasing `2.4.0-beta` you'll need to condense it to `2.4.0`

- [ ] Update the changelog `## Unreleased` section to the new version `CHANGELOG.md`  
       Make sure to leave the `## Unreleased` line unchanged for easier merges

- [ ] Ensure all GitHub API credentials from the `chatterino-ci` user are still valid

## After the PR has been merged

- [ ] Tag the release
- [ ] Manually run the [create-installer](https://github.com/Chatterino/chatterino2/actions/workflows/create-installer.yml) workflow.  
       This is only necessary if the tag was created after the CI in the main branch finished.

## After the binaries have been uploaded to fourtf's bucket

- [ ] Re-run the Publish Homebrew Cask on Release action
- [ ] Update links in the Chatterino website to point to the new release
