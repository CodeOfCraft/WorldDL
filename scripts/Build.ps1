param([ValidateSet('release', 'debug')][string]$Mode = 'release')

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$xmakeCommand = Get-Command xmake -ErrorAction SilentlyContinue
$xmakePath = if ($xmakeCommand) { $xmakeCommand.Source } else {
    Join-Path $projectRoot '.tools/xmake/xmake/xmake.exe'
}
if (!(Test-Path -LiteralPath $xmakePath)) {
    throw 'Install xmake 3.0 or later and LLVM 22 before building.'
}
$oldPath = $env:PATH
$oldGitCount = $env:GIT_CONFIG_COUNT
$gitIndex = if ($oldGitCount) { [int]$oldGitCount } else { 0 }
$keyPath = "env:GIT_CONFIG_KEY_$gitIndex"
$valuePath = "env:GIT_CONFIG_VALUE_$gitIndex"
try {
    $portableLlvm = Join-Path $projectRoot '.tools/clang+llvm-22.1.0-x86_64-pc-windows-msvc/bin'
    if (!(Get-Command clang-cl -ErrorAction SilentlyContinue) -and (Test-Path -LiteralPath $portableLlvm)) {
        $env:PATH = $portableLlvm + ';' + $env:PATH
    }
    Set-Item -Path $keyPath -Value 'core.longpaths'
    Set-Item -Path $valuePath -Value 'true'
    $env:GIT_CONFIG_COUNT = [string]($gitIndex + 1)
    Push-Location $projectRoot
    try {
        & $xmakePath f -y -p windows -a x64 -m $Mode --target_type=client
        if ($LASTEXITCODE -ne 0) { throw 'xmake configuration failed.' }
        & $xmakePath -y
        if ($LASTEXITCODE -ne 0) { throw 'xmake build failed.' }
    } finally {
        Pop-Location
    }
} finally {
    $env:PATH = $oldPath
    $env:GIT_CONFIG_COUNT = $oldGitCount
    Remove-Item -LiteralPath $keyPath, $valuePath -ErrorAction SilentlyContinue
}
