$ErrorActionPreference = 'Stop'
$tempRoot = [System.IO.Path]::GetTempPath()
$root = Join-Path $tempRoot ('worlddl-export-test-' + [guid]::NewGuid().ToString('N'))
$exportScript = Join-Path (Split-Path -Parent $PSScriptRoot) 'scripts/Export-World.ps1'
$lock = $null
try {
    $world = Join-Path $root 'World [test]'
    $db = Join-Path $world 'db'
    New-Item -ItemType Directory -Path $db -Force | Out-Null
    [System.IO.File]::WriteAllBytes((Join-Path $world 'level.dat'), [byte[]](10, 0, 0, 0, 4, 0, 0, 0, 10, 0, 0, 0))
    [System.IO.File]::WriteAllText((Join-Path $db 'CURRENT'), 'MANIFEST-000001')
    $lockPath = Join-Path $db 'LOCK'
    [System.IO.File]::WriteAllText($lockPath, '')
    $output = & $exportScript -WorldPath $world
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($output)
    try {
        $names = @($archive.Entries | ForEach-Object { $_.FullName })
        if ('level.dat' -notin $names -or 'db/CURRENT' -notin $names -or 'db/LOCK' -in $names) {
            throw 'Archive must contain world files at its root and exclude the database lock.'
        }
    } finally { $archive.Dispose() }
    $refused = $false
    try { & $exportScript -WorldPath $world | Out-Null } catch { $refused = $true }
    if (!$refused) { throw 'Existing mcworld archives must not be overwritten.' }
    $lock = [System.IO.File]::Open($lockPath, 'Open', 'ReadWrite', 'None')
    $refused = $false
    $lockedOutput = Join-Path $root 'locked.mcworld'
    try { & $exportScript -WorldPath $world -OutputPath $lockedOutput | Out-Null } catch { $refused = $true }
    if (!$refused -or (Test-Path -LiteralPath $lockedOutput)) {
        throw 'An actively locked world must not be exported.'
    }
    Write-Output 'WorldDL archive tests passed'
} finally {
    if ($lock) { $lock.Dispose() }
    $resolvedRoot = [System.IO.Path]::GetFullPath($root)
    if (!$resolvedRoot.StartsWith([System.IO.Path]::GetFullPath($tempRoot), [System.StringComparison]::OrdinalIgnoreCase)) {
        throw 'Test cleanup path must remain in the temporary directory.'
    }
    if (Test-Path -LiteralPath $resolvedRoot) { Remove-Item -LiteralPath $resolvedRoot -Recurse -Force }
}
