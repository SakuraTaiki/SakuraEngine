param(
    [Parameter(Mandatory = $true)]
    [string]$Script,

    [string[]]$ScriptArgs = @()
)

$ErrorActionPreference = 'Stop'

$blenderExe = 'C:\Program Files\Blender Foundation\Blender 4.4\blender.exe'
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$scriptPath = if ([System.IO.Path]::IsPathRooted($Script)) {
    $Script
} else {
    Join-Path $workspaceRoot $Script
}
$resolvedScript = (Resolve-Path -LiteralPath $scriptPath).Path

if (-not $resolvedScript.StartsWith($workspaceRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'Blender scripts must be inside the CG2 workspace.'
}

$arguments = @('--background', '--python', $resolvedScript)
if ($ScriptArgs.Count -gt 0) {
    $arguments += '--'
    $arguments += $ScriptArgs
}

& $blenderExe @arguments
if ($LASTEXITCODE -ne 0) {
    throw "Blender exited with code $LASTEXITCODE."
}
