param (
    [string] $InstallDir = "Chatterino2"
)

if ($null -eq $Env:VCToolsRedistDir) {
    Write-Error "VCToolsRedistDir is not set. Forgot to set Visual Studio environment variables?";
    exit 1
}

# A path to the runtime libraries (e.g. "$Env:VCToolsRedistDir\onecore\x64\Microsoft.VC143.CRT")
$vclibs = (Get-ChildItem "$Env:VCToolsRedistDir\onecore\x64" -Filter '*.CRT')[0].FullName;

# All executables and libraries in the installation directory
$targets = Get-ChildItem -Recurse -Include '*.dll', '*.exe' $InstallDir;
# All dependencies of the targets (with duplicates)
$all_deps = $targets | ForEach-Object { (dumpbin /DEPENDENTS $_.FullName) -match '^(?!Dump of).+\.dll$' } | ForEach-Object { $_.Trim() };
# All dependencies without duplicates
$dependencies = $all_deps | Sort-Object -Unique;

$n_deployed = 0;
foreach ($dll in $dependencies) {
    Write-Output "Checking for $dll";
    if (Test-Path -PathType Leaf "$vclibs\$dll") {
        Write-Output "Deploying $dll";
        Copy-Item "$vclibs\$dll" "$InstallDir\$dll" -Force;
        $n_deployed++;
    }
}

Write-Output "Deployed $n_deployed libraries";
