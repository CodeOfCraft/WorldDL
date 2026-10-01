param(
    [Parameter(Mandatory = $true)][string]$WorldPath,
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
$world = (Resolve-Path -LiteralPath $WorldPath).Path
if (!(Test-Path -LiteralPath (Join-Path $world 'level.dat') -PathType Leaf) -or
    !(Test-Path -LiteralPath (Join-Path $world 'db') -PathType Container)) {
    throw 'WorldPath must contain level.dat and a db directory.'
}
if (!$OutputPath) { $OutputPath = $world + '.mcworld' }
$output = [System.IO.Path]::GetFullPath($OutputPath)
if ([System.IO.Path]::GetExtension($output) -ne '.mcworld') {
    throw 'OutputPath must end in .mcworld.'
}
if (Test-Path -LiteralPath $output) { throw 'Output already exists.' }
if ($output.StartsWith($world.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar,
                       [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'Output must be outside the world directory.'
}
$lockPath = [System.IO.Path]::GetFullPath((Join-Path $world 'db/LOCK'))
$lockStream = $null
$zipPath = Join-Path ([System.IO.Path]::GetDirectoryName($output)) (
    [System.IO.Path]::GetRandomFileName() + '.zip')
try {
    if (Test-Path -LiteralPath $lockPath) {
        $lockStream = [System.IO.File]::Open($lockPath, [System.IO.FileMode]::Open,
            [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
    }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::Open($zipPath, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($file in Get-ChildItem -LiteralPath $world -Recurse -File) {
            if ($file.FullName -eq $lockPath) { continue }
            $entry = [System.IO.Path]::GetRelativePath($world, $file.FullName).Replace('\', '/')
            [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $archive, $file.FullName, $entry, [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
        }
    } finally {
        $archive.Dispose()
    }
    Move-Item -LiteralPath $zipPath -Destination $output
    Write-Output $output
} finally {
    if ($lockStream) { $lockStream.Dispose() }
    if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath }
}
