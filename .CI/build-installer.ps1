if (-not (Test-Path -PathType Container Chatterino2)) {
    Write-Error "Couldn't find a folder called 'Chatterino2' in the current directory.";
    exit 1
}

# Check if we're on a tag
$OldErrorActionPref = $ErrorActionPreference;
$ErrorActionPreference = 'Continue';
git describe --exact-match --match 'v*' *> $null;
$isTagged = $?;
$ErrorActionPreference = $OldErrorActionPref;

$defines = $null;
if ($isTagged) {
    # This is a release.
    # Make sure, any existing `modes` file is overwritten for the user,
    # for example when updating from nightly to stable.
    Write-Output "" | Out-File Chatterino2/modes -Encoding ASCII;
    $installerBaseName = "Chatterino.Installer";
}
else {
    Write-Output nightly | Out-File Chatterino2/modes -Encoding ASCII;
    $defines = "/DIS_NIGHTLY=1";
    $installerBaseName = "Chatterino.Nightly.Installer";
}

if ($Env:GITHUB_OUTPUT) {
    # This is used in CI when creating the artifact
    "C2_INSTALLER_BASE_NAME=$installerBaseName" >> "$Env:GITHUB_OUTPUT"
}

# Copy vc_redist.x64.exe
if ($null -eq $Env:VCToolsRedistDir) {
    Write-Error "VCToolsRedistDir is not set. Forgot to set Visual Studio environment variables?";
    exit 1
}
Copy-Item "$Env:VCToolsRedistDir\vc_redist.x64.exe" .;

$VCRTVersion = (Get-Item "$Env:VCToolsRedistDir\vc_redist.x64.exe").VersionInfo;

# Build the installer
ISCC `
    /DWORKING_DIR="$($pwd.Path)\" `
    /DINSTALLER_BASE_NAME="$installerBaseName" `
    /DSHIPPED_VCRT_MINOR="$($VCRTVersion.FileMinorPart)" `
    /DSHIPPED_VCRT_VERSION="$($VCRTVersion.FileDescription)" `
    $defines `
    /O. `
    "$PSScriptRoot\chatterino-installer.iss";

Move-Item "$installerBaseName.exe" "$installerBaseName$($Env:VARIANT_SUFFIX).exe"
