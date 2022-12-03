# Checklist for making a release

- [ ] Updated version code in `src/common/Version.hpp`
- [ ] Updated version code in `CMakeLists.txt`  
       This can only be "whole versions", so if you're releasing `2.4.0-beta` you'll need to condense it to `2.4.0`
- [ ] Updated version code in `resources/com.chatterino.chatterino.appdata.xml`  
       This cannot use dash to denote a pre-release identifier, you have to use a tilde instead.
- [ ] Update the changelog `## Unreleased` section to the new version `CHANGELOG.md`  
       Make sure to leave the `## Unreleased` line unchanged for easier merges
- [ ] Push directly to master :tf:
