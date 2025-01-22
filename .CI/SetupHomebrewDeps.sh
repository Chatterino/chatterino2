#!/usr/bin/env bash

set -e

# Prefix for where to find the ARM64 library
arm64_homebrew_dir="/opt/homebrew"
# Prefix for where to find the x86 library
x86_64_homebrew_dir="/opt/homebrew-x86_64"
 # Directory where we place the finished universal library
universal_lib_dir="/opt/universal-lib"

# args: path-to-library (in homebrew dir)
c2-make-universal-dylib() {
    local _input_lib="$1"
    if [ -z "${_input_lib}" ]; then
        echo "usage: $0 [lib-path-relative-to-homebrew] (e.g. $0 lib/libboost_random-mt.dylib)"
        exit 1
    fi

    if [ ! -w "${universal_lib_dir}" ]; then
        echo "error: The current user does not have write permission in the universal lib directory (${universal_lib_dir})"
        exit 1
    fi

    local _input_lib_filename="$(basename "${_input_lib}")"

    local _arm64_lib="${arm64_homebrew_dir}/${_input_lib}"
    local _x86_64_lib="${x86_64_homebrew_dir}/${_input_lib}"
    local _override_lib=$(realpath "${arm64_homebrew_dir}/${_input_lib}")

    local _universal_lib="${universal_lib_dir}/${_input_lib_filename}"

    if [ ! -f "${_arm64_lib}" ]; then
        echo "error: The ARM64 library '${_input_lib}' cannot be found at '${_arm64_lib}'"
        exit 1
    fi

    if [ ! -f "${_x86_64_lib}" ]; then
        echo "error: The x86_64 library '${_input_lib}' cannot be found at '${_x86_64_lib}'"
        exit 1
    fi

    # Create the combined library
    if ! lipo "${_arm64_lib}" "${_x86_64_lib}" -create -output "${_universal_lib}"; then
        echo "error: Something went wrong creating the combined library"
        echo "Some errors can be solved by re-linking the original libraries (e.g. brew link --overwrite boost)"
        exit 1
    fi

    echo "Created the combined library at '${_universal_lib}"

    # Override
    rm -v "${_override_lib}"
    ln -v -s "${_universal_lib}" "${_override_lib}"
}

sudo mkdir "$x86_64_homebrew_dir"
sudo mkdir "$universal_lib_dir"

sudo chown -R $USER "$universal_lib_dir"

echo "Installing x86_64 brew"
sudo curl -L https://github.com/Homebrew/brew/tarball/master | sudo tar xz --strip 1 -C "$x86_64_homebrew_dir"
sudo chown -R $USER "$x86_64_homebrew_dir"

echo "Installing ARM dependencies"
brew install "$@"

echo "Installing x86_64 dependencies"
for dep in "$@"
do
    arch -x86_64 "$x86_64_homebrew_dir/bin/brew" fetch --force --bottle-tag=x86_64_ventura "$dep"
    arch -x86_64 "$x86_64_homebrew_dir/bin/brew" install $(arch -x86_64 "$x86_64_homebrew_dir/bin/brew" --cache --bottle-tag=x86_64_ventura "$dep")
done

echo "Relinking boost libraries"
c2-make-universal-dylib lib/libboost_random.dylib

echo "Relinking OpenSSL 3 libcrypto"
c2-make-universal-dylib lib/libcrypto.dylib
echo "Relinking OpenSSL 3 libssl"
c2-make-universal-dylib lib/libssl.dylib
