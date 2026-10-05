param([switch]$Package, [string]$IsccPath, [string]$BuildDirectory = 'build')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio Build Tools with Desktop development with C++.' }
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'MSVC C++ toolchain not found.' }
$devshell = Join-Path $vs 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Import-Module $devshell
Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
$env:VSLANG = '1033'
# Keep compiler diagnostics and Ninja include dependencies in UTF-8.
& chcp.com 65001 | Out-Null
$build = Join-Path $root $BuildDirectory
& cmake -S $root -B $build -G Ninja -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE) { throw 'CMake configure failed.' }
& cmake --build $build --parallel 2
if ($LASTEXITCODE) { throw 'Build failed.' }
& ctest --test-dir $build --output-on-failure
if ($LASTEXITCODE) { throw 'Tests failed.' }
$output = Join-Path $root 'artifacts'
New-Item -ItemType Directory -Path $output -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $build 'DeskHide.exe') -Destination $output
if ($Package) {
    if (!$IsccPath) {
        $compiler = Get-Command ISCC.exe -ErrorAction SilentlyContinue
        if ($compiler) { $IsccPath = $compiler.Source }
    }
    if (!$IsccPath -or !(Test-Path -LiteralPath $IsccPath)) { throw 'Pass -IsccPath pointing to Inno Setup 6 ISCC.exe.' }
    & $IsccPath (Join-Path $root 'installer\setup.iss')
    if ($LASTEXITCODE) { throw 'Installer compilation failed.' }
    $portable = @('artifacts\DeskHide.exe', 'README.md', 'README.zh-CN.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md', 'docs') | ForEach-Object { Join-Path $root $_ }
    Compress-Archive -LiteralPath $portable -DestinationPath (Join-Path $output 'DeskHide-1.0.0-x64-portable.zip') -Force
    $sources = @('.gitignore', 'AGENTS.md', 'CMakeLists.txt', 'LICENSE', 'README.md', 'README.zh-CN.md', 'THIRD_PARTY_NOTICES.md', 'src', 'assets', 'tests', 'scripts', 'installer', 'docs', '.github') | ForEach-Object { Join-Path $root $_ }
    Compress-Archive -LiteralPath $sources -DestinationPath (Join-Path $output 'DeskHide-1.0.0-source.zip') -Force
    Get-ChildItem -LiteralPath $output -File | Where-Object Extension -In '.exe', '.zip' | Get-FileHash -Algorithm SHA256 |
        ForEach-Object { $_.Hash.ToLowerInvariant() + '  ' + (Split-Path $_.Path -Leaf) } |
        Set-Content -LiteralPath (Join-Path $output 'SHA256SUMS.txt') -Encoding UTF8
}
