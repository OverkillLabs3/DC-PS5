# Builds DeadCells-PS5-Native-<version>-win64.zip (+ SHA256SUMS.txt): the launcher (deadcells.exe), the system libraries, the relinker and a README.
# The package contains no game files and needs nothing to be installed.
#
# Prerequisite: the project is built (cmake --build <BuildDir> --target libs relinker deadcells).
# Usage: powershell -ExecutionPolicy Bypass -File tools\package_release.ps1 [-BuildDir <dir>] [-Version 0.1.0] [-Sign] [-CertSubject <text>]
#
# Signing (-Sign): signs deadcells.exe and tools\relinker.exe with a code-signing certificate from the current user's
# certificate store (the one whose subject contains -CertSubject, or the newest code-signing certificate), with a timestamp.
# A certificate from a public CA makes Windows show your name as publisher; a self-signed one only makes the files
# tamper-evident and does not remove the SmartScreen warning. The private key stays in your certificate store; never commit it.
param(
    [string]$BuildDir = "",
    [string]$Version = "0.1.0",
    [switch]$Sign,
    [string]$CertSubject = "",
    [string]$TimestampServer = "http://timestamp.digicert.com"
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot

if (-not $BuildDir) {
    foreach ($candidate in "$repo\build", (Join-Path (Split-Path -Parent $repo) "build")) {
        if (Test-Path "$candidate\core\libs\libs") { $BuildDir = $candidate; break }
    }
}
if (-not $BuildDir -or -not (Test-Path "$BuildDir\core\libs\libs")) { throw "Build directory not found. Pass -BuildDir <dir> (built with the 'libs', 'relinker' and 'deadcells' targets)." }

# Runtime DLLs of the MinGW toolchain that built the libraries (found next to g++ on PATH, or in the workspace toolchain folder).
$gpp = (Get-Command g++ -ErrorAction SilentlyContinue).Source
if (-not $gpp) { $gpp = Join-Path (Split-Path -Parent $repo) "tools\mingw64\bin\g++.exe" }
$mingwBin = Split-Path -Parent $gpp

$releaseRoot = Join-Path $repo "release"
$out = Join-Path $releaseRoot "DeadCells-PS5-Native-$Version-win64"
if (Test-Path $out) { Remove-Item -Recurse -Force $out }
New-Item -ItemType Directory -Force "$out\libs", "$out\tools" | Out-Null

Copy-Item "$BuildDir\deadcells.exe" $out
Copy-Item "$BuildDir\core\relinker\relinker.exe" "$out\tools"
Copy-Item "$BuildDir\core\libs\libs\*.prx" "$out\libs"
# the native NGS2 engine replaces the stub under the stub's name
Copy-Item "$BuildDir\core\libs\libs\libSceNgs2.native.prx" "$out\libs\libSceNgs2.prx" -Force
Remove-Item "$out\libs\libSceNgs2.native.prx", "$out\libs\libSceAudiodec.native.prx" -ErrorAction SilentlyContinue
foreach ($d in "libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll") { Copy-Item "$mingwBin\$d" "$out\libs" }
Copy-Item "$repo\LICENSE" $out
Copy-Item "$repo\tools\release\README.txt" $out

if ($Sign) {
    $certs = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Where-Object { -not $CertSubject -or $_.Subject -like "*$CertSubject*" } | Sort-Object NotBefore -Descending
    if (-not $certs) { throw "No code-signing certificate found in Cert:\CurrentUser\My." }
    $cert = $certs[0]
    foreach ($file in "$out\deadcells.exe", "$out\tools\relinker.exe") {
        $result = Set-AuthenticodeSignature -FilePath $file -Certificate $cert -HashAlgorithm SHA256 -TimestampServer $TimestampServer
        if (-not $result.SignerCertificate) { throw "Signing failed for $file : $($result.StatusMessage)" }
        Write-Output "signed $(Split-Path -Leaf $file) with '$($cert.Subject)' ($($result.Status))"
    }
}

# Safety net: refuse to ship anything that looks like game content.
$bad = Get-ChildItem $out -Recurse -File | Where-Object { $_.Name -match '^(eboot|game\.exe|dc_runtime|res\.pak|param\.json)' -or $_.Extension -in '.elf', '.bin', '.pak', '.sprx' }
if ($bad) { throw "Game-like files in package: $($bad.FullName -join ', ')" }

$zip = "$out.zip"
Compress-Archive -Path "$out\*" -DestinationPath $zip -Force
$hash = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
"$hash  $(Split-Path -Leaf $zip)" | Set-Content (Join-Path $releaseRoot "SHA256SUMS.txt") -Encoding ascii
Get-Item $zip | Select-Object Name, Length
Write-Output "SHA256 $hash"
