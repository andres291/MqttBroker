param(
    [ValidateSet('Build', 'Upload', 'Monitor')]
    [string]$Mode = 'Build',
    [string]$Port = ''
)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
# Keep command-line builds separate from background IDE metadata/clean actions.
$env:PLATFORMIO_BUILD_DIR = Join-Path $projectRoot '.pio\build-starter' 
$projectPio = Join-Path $projectRoot '.pio-core\penv\Scripts\pio.exe'
$projectPython = Join-Path $projectRoot '.pio-core\penv\Scripts\python.exe'

if (-not (Test-Path -LiteralPath $projectPio)) {
    # Package installation bootstraps the pinned platform's own Python/Core.
    # Do not build with the global Core: its SCons version may differ.
    & pio pkg install --project-dir $projectRoot
    if ($LASTEXITCODE -ne 0) { throw 'Project tool installation failed.' }
    if (-not (Test-Path -LiteralPath $projectPio)) {
        throw 'The project-specific PlatformIO executable was not installed.'
    }
}

if ($Mode -eq 'Monitor') {
    $pioArguments = @('device', 'monitor', '--project-dir', $projectRoot)
    if ($Port) { $pioArguments += @('--port', $Port) }
} else {
    $pioArguments = @('run', '--project-dir', $projectRoot)
    if ($Mode -eq 'Upload') {
        $pioArguments += @('--target', 'upload')
        if ($Port) { $pioArguments += @('--upload-port', $Port) }
    }
}
& $projectPython -m platformio @pioArguments
exit $LASTEXITCODE
