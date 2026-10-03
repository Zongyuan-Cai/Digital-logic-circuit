[CmdletBinding()]
param(
    [switch]$Stop,
    [switch]$Status,
    [switch]$Setup,
    [switch]$Rebuild,
    [switch]$Help,
    [ValidateRange(1, 65535)][int]$BackendPort = 8000,
    [ValidateRange(1, 65535)][int]$FrontendPort = 5173,
    [string]$Python = '',
    [string]$Generator = 'Visual Studio 17 2022'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'scripts/windows-common.ps1')

try {
    if (@($Stop, $Status, $Setup, $Help).Where({ $_ }).Count -gt 1) {
        throw 'Use only one of -Stop, -Status, -Setup, or -Help.'
    }
    if ($Help) {
        Write-Host @'
Logic Lab - native Windows launcher (PowerShell 5.1+)
  .\start.cmd                 Set up and start backend + frontend
  .\start.cmd -Setup          Install project dependencies and build the core
  .\start.cmd -Rebuild        Rebuild the core, then start
  .\start.cmd -Status         Show service status
  .\start.cmd -Stop           Stop services launched by this script
  .\start.cmd -Python C:\Python312\python.exe
  .\start.cmd -BackendPort 8001 -FrontendPort 5174

Requires: 64-bit CPython 3.10+, Node.js 24 LTS (or 22.13+), CMake 3.21+,
Visual Studio 2022 C++ build tools with the Windows SDK. Git is needed for tests.
Use a local Windows checkout, such as C:\work\logic-lab.
Build output: build\windows; Python environment: .venv-windows
Logs: .logs\windows; process records: .pids\windows
'@
        exit 0
    }
    if ($Stop) {
        Stop-ManagedService 'frontend'
        Stop-ManagedService 'backend'
        exit 0
    }
    if ($Status) {
        Show-WindowsServiceStatus $BackendPort $FrontendPort
        exit 0
    }
    if ($BackendPort -eq $FrontendPort) { throw 'BackendPort and FrontendPort must differ.' }
    Assert-WindowsWorkspace
    $backendProcess = Get-ManagedProcess 'backend'
    $frontendProcess = Get-ManagedProcess 'frontend'
    if ($backendProcess -or $frontendProcess) {
        if ($Setup -or $Rebuild -or -not ($backendProcess -and $frontendProcess)) {
            throw 'Stop managed services with .\start.cmd -Stop before setup, rebuilding, or recovering a partial start.'
        }
        Assert-ServicePort 'backend' $BackendPort
        Assert-ServicePort 'frontend' $FrontendPort
        Wait-ServiceReady "http://127.0.0.1:$BackendPort/api/health" 'backend' -CoreRequired
        Wait-ServiceReady "http://127.0.0.1:$FrontendPort" 'frontend'
        Write-Host "Already running: http://localhost:$FrontendPort"
        exit 0
    }
    # Setup does not need ports; starting refuses to touch external listeners.
    if (-not $Setup) {
        Assert-ServicePort 'backend' $BackendPort
        Assert-ServicePort 'frontend' $FrontendPort
    }
    Initialize-WindowsEnvironment -Python $Python -Generator $Generator -Rebuild:$Rebuild
    if ($Setup) { Write-Host 'Setup completed.'; exit 0 }

    $started = @()
    $previousBuildDir = $env:LOGIC_SIM_BUILD_DIR
    $previousApiUrl = $env:LOGIC_LAB_API_URL
    $previousEncoding = $env:PYTHONIOENCODING
    try {
        $env:LOGIC_SIM_BUILD_DIR = $script:ModuleDir
        $env:LOGIC_LAB_API_URL = "http://127.0.0.1:$BackendPort"
        $env:PYTHONIOENCODING = 'utf-8'
        if (-not (Get-ManagedProcess 'backend')) {
            Start-ManagedService 'backend' $script:VenvPython @('-m', 'uvicorn', 'app.main:app', '--host', '127.0.0.1', '--port', "$BackendPort") (Join-Path $script:ProjectRoot 'backend') $BackendPort
            $started += 'backend'
        }
        Wait-ServiceReady "http://127.0.0.1:$BackendPort/api/health" 'backend' -CoreRequired
        if (-not (Get-ManagedProcess 'frontend')) {
            $vite = Join-Path $script:ProjectRoot 'frontend/node_modules/vite/bin/vite.js'
            Start-ManagedService 'frontend' $script:Node @($vite, '--host', '127.0.0.1', '--port', "$FrontendPort", '--strictPort') (Join-Path $script:ProjectRoot 'frontend') $FrontendPort
            $started += 'frontend'
        }
        Wait-ServiceReady "http://127.0.0.1:$FrontendPort" 'frontend'
    } catch {
        foreach ($service in $started) { Stop-ManagedService $service }
        throw
    } finally {
        $env:LOGIC_SIM_BUILD_DIR = $previousBuildDir
        $env:LOGIC_LAB_API_URL = $previousApiUrl
        $env:PYTHONIOENCODING = $previousEncoding
    }
    Write-Host "Frontend: http://localhost:$FrontendPort"
    Write-Host "API docs: http://localhost:$BackendPort/docs"
    Write-Host 'Stop: .\start.cmd -Stop'
    exit 0
} catch {
    Write-Host "[ERROR] $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
