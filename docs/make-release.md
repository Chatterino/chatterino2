# Checklist for making a release

## In the release PR

- [ ] Updated version code in `src/common/Version.hpp`
- [ ] Updated version code in `CMakeLists.txt`  
       This can only be "whole versions", so if you're releasing `2.4.0-beta` you'll need to condense it to `2.4.0`
- [ ] Add a new release at the top of the `releases` key in `resources/com.chatterino.chatterino.appdata.xml`  
       This cannot use dash to denote a pre-release identifier, you have to use a tilde instead.

- [ ] Updated version code in `.CI/chatterino-installer.iss`
- [ ] Update the changelog `## Unreleased` section to the new version `CHANGELOG.md`  
       Make sure to leave the `## Unreleased` line unchanged for easier merges

## After the PR has been merged

- [ ] Tag the release
- [ ] Manually run the [create-installer](https://github.com/Chatterino/chatterino2/actions/workflows/create-installer.yml) workflow.  
       This is only necessary if the tag was created after the CI in the main branch finished.
- [ ] Start a manual [Launchpad import](https://code.launchpad.net/~pajlada/chatterino/+git/chatterino) - scroll down & click Import Now
- [ ] Make a PPA release to [this repo](https://git.launchpad.net/~pajlada/+git/chatterino-packaging/) with the `debchange` command.  
       `debchange -v 2.4.0` then add the changelog entries  
       `debchange --release` then change the distro to be `unstable`
