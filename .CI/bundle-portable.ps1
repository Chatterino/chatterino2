if (-not (Test-Path -PathType Container Chatterino2)) {
    Write-Error "Couldn't find a folder called 'Chatterino2' in the current directory.";
    exit 1
}
if (-not $Env:C2_PORTABLE_INSTALLER_VERSION -or -not $Env:C2_PORTABLE_INSTALLER_SHA256) {
    Write-Error "C2_PORTABLE_INSTALLER_VERSION or C2_PORTABLE_INSTALLER_SHA256 not defined.";
    exit 1
}

function Remove-IfExists {
    param (
        [string] $Path
    )
    if (Test-Path -PathType Container $Path) {
        Remove-Item $Path -Force -Recurse -Confirm:$false;
    }
    elseif (Test-Path -PathType Leaf $Path) {
        Remove-Item $Path -Force;
    }
}

# Check if we're on a tag
$OldErrorActionPref = $ErrorActionPreference;
$ErrorActionPreference = 'Continue';
git describe --exact-match --match 'v*' *> $null;
$isTagged = $?;
$ErrorActionPreference = $OldErrorActionPref;

Write-Output portable | Out-File Chatterino2/modes -Encoding ASCII;
if ($isTagged) {
    # This is a release.
    # Make sure, any existing `modes` file is overwritten for the user,
    # for example when updating from nightly to stable.
    $bundleBaseName = "Chatterino7.Portable";
}
else {
    Write-Output nightly | Out-File Chatterino2/modes -Append -Encoding ASCII;
    $bundleBaseName = "Chatterino7.Nightly.Portable";
}

if ($Env:GITHUB_OUTPUT) {
    # This is used in CI when creating the artifact
    "C2_PORTABLE_BASE_NAME=$bundleBaseName" >> "$Env:GITHUB_OUTPUT"
}

Remove-IfExists "Chatterino2/updater.1";
New-Item "Chatterino2/updater.1" -ItemType Directory;

Invoke-RestMethod "https://github.com/Nerixyz/c2-portable-updater/releases/download/$($Env:C2_PORTABLE_INSTALLER_VERSION)/c2-portable-updater-x86_64-pc-windows-msvc.zip" -OutFile _portable-installer.zip;
$updaterHash = (Get-FileHash _portable-installer.zip).Hash.ToLower();
if (-not $updaterHash -eq $Env:C2_PORTABLE_INSTALLER_SHA256) {
    Write-Error "Hash mismatch: expected $($Env:C2_PORTABLE_INSTALLER_SHA256) - got: $updaterHash";
    exit 1
}

7z e -y _portable-installer.zip c2-portable-updater.exe;
Move-Item c2-portable-updater.exe "Chatterino2/updater.1/ChatterinoUpdater.exe" -Force;
7z e -so _portable-installer.zip LICENSE-MIT > "Chatterino2/updater.1/LICENSE";
Remove-IfExists _portable-installer.zip;

Remove-IfExists "$bundleBaseName$($Env:VARIANT_SUFFIX).zip";
7z a "$bundleBaseName$($Env:VARIANT_SUFFIX).zip" Chatterino2/;
