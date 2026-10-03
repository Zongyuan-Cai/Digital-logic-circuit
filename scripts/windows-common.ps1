# Shared native Windows setup and service lifecycle. Compatible with PowerShell 5.1.
Set-StrictMode -Version Latest
$script:ProjectRoot = Split-Path $PSScriptRoot -Parent
$script:BuildDir = Join-Path $script:ProjectRoot 'build/windows'
$script:ModuleDir = Join-Path $script:BuildDir 'python'
$script:VenvPython = Join-Path $script:ProjectRoot '.venv-windows/Scripts/python.exe'
$script:PidDir = Join-Path $script:ProjectRoot '.pids/windows'
$script:LogDir = Join-Path $script:ProjectRoot '.logs/windows'

function Assert-WindowsWorkspace {
    if ($env:OS -ne 'Windows_NT') { throw 'Use start.sh on Linux or WSL.' }
    if ($script:ProjectRoot.StartsWith('\\')) {
        throw 'Native Windows builds require a local drive. Copy/clone this project to C:\work\logic-lab (without build, node_modules, or virtual environments), or keep using ./start.sh inside WSL.'
    }
}

function Invoke-Checked {
    param([string]$FilePath, [string[]]$Arguments, [string]$WorkingDirectory = $script:ProjectRoot, [switch]$Capture)
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $FilePath
    $info.Arguments = ($Arguments | ForEach-Object { ConvertTo-ProcessArgument $_ }) -join ' '
    if ([IO.Path]::GetExtension($FilePath) -in @('.cmd', '.bat')) {
        $info.FileName = $env:ComSpec
        $info.Arguments = '/d /s /c "' + (ConvertTo-ProcessArgument $FilePath) + ' ' + $info.Arguments + '"'
    }
    $info.WorkingDirectory = $WorkingDirectory
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = [Text.Encoding]::UTF8
    $info.StandardErrorEncoding = [Text.Encoding]::UTF8
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $info
    try {
        $null = $process.Start()
        # Drain both pipes concurrently to avoid deadlock on compiler/pip output.
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        if (-not $Capture) {
            if ($stdout) { Write-Host $stdout.TrimEnd() }
            if ($stderr) { Write-Host $stderr.TrimEnd() }
        }
        if ($process.ExitCode -ne 0) { throw "Command failed ($($process.ExitCode)): $FilePath $($Arguments -join ' ')`n$stderr`n$stdout" }
        if ($Capture) { return ($stdout.TrimEnd() -split '\r?\n') }
    } finally { $process.Dispose() }
}

function Resolve-Python {
    param([string]$Requested)
    if ($Requested) {
        $command = Get-Command $Requested -ErrorAction SilentlyContinue
        if (-not $command) { throw "Python executable not found: $Requested" }
        $executable = $command.Source
    } else {
        $launcher = Get-Command py.exe -ErrorAction SilentlyContinue
        if ($launcher) {
            $executable = [string](Invoke-Checked $launcher.Source @('-3', '-c', 'import sys; print(sys.executable)') -Capture | Select-Object -Last 1)
        } else {
            $command = Get-Command python.exe -ErrorAction SilentlyContinue
            if (-not $command -or $command.Source -like '*\WindowsApps\*') {
                throw 'Install 64-bit CPython 3.10+ from python.org, or pass -Python C:\path\python.exe. Microsoft Store execution aliases are not a Python installation.'
            }
            $executable = $command.Source
        }
    }
    $info = (Invoke-Checked $executable @('-c', 'import json,sys,struct; print(json.dumps({"version":list(sys.version_info[:2]),"bits":struct.calcsize("P")*8,"platform":sys.platform,"implementation":sys.implementation.name}))') -Capture | Select-Object -Last 1) | ConvertFrom-Json
    if ($info.platform -ne 'win32' -or $info.implementation -ne 'cpython' -or $info.bits -ne 64 -or $info.version[0] -ne 3 -or $info.version[1] -lt 10) {
        throw 'Use native Windows 64-bit CPython 3.10+; the default MSVC build targets x64.'
    }
    return $executable
}

function Resolve-CMake {
    $command = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $vswhere = Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (Test-Path -LiteralPath $vswhere) {
        $install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($install) {
            $bundled = Join-Path ([string]$install) 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
            if (Test-Path -LiteralPath $bundled) { return $bundled }
        }
    }
    throw 'CMake not found. Install CMake 3.21+ and add it to PATH, or install the Visual Studio C++ CMake tools.'
}

function Initialize-WindowsEnvironment {
    param([string]$Python = '', [string]$Generator = 'Visual Studio 17 2022', [switch]$Rebuild, [switch]$BuildTests)
    Assert-WindowsWorkspace
    $nodeCommand = Get-Command node.exe -ErrorAction SilentlyContinue
    $npmCommand = Get-Command npm.cmd -ErrorAction SilentlyContinue
    if (-not $nodeCommand -or -not $npmCommand) { throw 'Install Node.js 24 LTS with npm, then reopen the terminal.' }
    $script:Node = $nodeCommand.Source
    $version = [version](([string](Invoke-Checked $script:Node @('--version') -Capture)).Trim().TrimStart('v'))
    if (-not (($version.Major -eq 20 -and $version.Minor -ge 19) -or ($version.Major -eq 22 -and $version.Minor -ge 13) -or $version.Major -ge 24)) {
        throw 'The frontend requires Node.js 20.19+, 22.13+, or 24+. Node.js 24 LTS is recommended.'
    }
    $script:CMake = Resolve-CMake
    $cmakeVersion = [string](Invoke-Checked $script:CMake @('--version') -Capture | Select-Object -First 1)
    if ($cmakeVersion -notmatch 'version (\d+\.\d+\.\d+)' -or [version]$Matches[1] -lt [version]'3.21.0') {
        throw 'CMake 3.21+ is required for the Visual Studio 2022 generator.'
    }
    if ($BuildTests -and -not (Get-Command git.exe -ErrorAction SilentlyContinue)) { throw 'Install Git for Windows and add it to PATH to fetch GoogleTest.' }
    $basePython = Resolve-Python $Python
    if (-not (Test-Path -LiteralPath $script:VenvPython)) {
        Write-Host '[setup] Creating .venv-windows'
        Invoke-Checked $basePython @('-m', 'venv', (Join-Path $script:ProjectRoot '.venv-windows'))
    }
    $expectedBase = [string](Invoke-Checked $basePython @('-c', 'import sys; print(sys.base_prefix)') -Capture | Select-Object -Last 1)
    $actualBase = [string](Invoke-Checked $script:VenvPython @('-c', 'import sys; print(sys.base_prefix)') -Capture | Select-Object -Last 1)
    if (-not $actualBase.Equals($expectedBase, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'The existing .venv-windows uses a different Python installation. Stop services and recreate .venv-windows and build/windows before switching Python.'
    }
    # Keep the interpreter and import libraries bound to this project environment.
    Invoke-Checked $script:VenvPython @('-c', 'import sys; assert sys.platform == "win32" and sys.version_info >= (3,10)')
    $requirements = Join-Path $script:ProjectRoot 'backend/requirements.txt'
    $dependencyHash = (Get-FileHash -LiteralPath $requirements -Algorithm SHA256).Hash + ':pybind11>=2.10'
    $dependencyStamp = Join-Path $script:ProjectRoot '.venv-windows/dependencies.txt'
    if (-not (Test-Path -LiteralPath $dependencyStamp) -or (Get-Content -LiteralPath $dependencyStamp -Raw).Trim() -ne $dependencyHash) {
        Write-Host '[setup] Installing backend and binding dependencies'
        Invoke-Checked $script:VenvPython @('-m', 'pip', 'install', '-r', $requirements, 'pybind11>=2.10')
        Set-Content -LiteralPath $dependencyStamp -Value $dependencyHash -Encoding UTF8
    }
    $pybindDir = [string](Invoke-Checked $script:VenvPython @('-m', 'pybind11', '--cmakedir') -Capture | Select-Object -Last 1)
    $testing = if ($BuildTests) { 'ON' } else { 'OFF' }
    $configure = @('-S', (Join-Path $script:ProjectRoot 'sim-core'), '-B', $script:BuildDir, '-G', $Generator, "-DPython_EXECUTABLE=$script:VenvPython", "-Dpybind11_DIR=$pybindDir", "-DBUILD_TESTING=$testing")
    if ($Generator.StartsWith('Visual Studio')) { $configure += @('-A', 'x64') }
    Write-Host '[setup] Configuring and building the native simulation core'
    Invoke-Checked $script:CMake $configure
    $build = @('--build', $script:BuildDir, '--config', 'Release', '--parallel')
    if (-not $BuildTests) { $build += @('--target', 'logic_sim') }
    if ($Rebuild) { $build += '--clean-first' }
    Invoke-Checked $script:CMake $build
    $previousModule = $env:LOGIC_SIM_BUILD_DIR
    try {
        $env:LOGIC_SIM_BUILD_DIR = $script:ModuleDir
        Invoke-Checked $script:VenvPython @('-c', 'import os,sys; sys.path.insert(0,os.environ["LOGIC_SIM_BUILD_DIR"]); import logic_sim; print("[setup] Native module:",logic_sim.__file__)')
    } finally { $env:LOGIC_SIM_BUILD_DIR = $previousModule }
    $frontend = Join-Path $script:ProjectRoot 'frontend'
    $frontendStamp = Join-Path $frontend 'node_modules/.logic-lab-windows'
    $frontendHash = (Get-FileHash -LiteralPath (Join-Path $frontend 'package-lock.json') -Algorithm SHA256).Hash + ":win32:$($version.Major)"
    if (-not (Test-Path -LiteralPath $frontendStamp) -or (Get-Content -LiteralPath $frontendStamp -Raw).Trim() -ne $frontendHash) {
        Write-Host '[setup] Installing Windows frontend dependencies'
        Invoke-Checked $npmCommand.Source @('ci') $frontend
        Set-Content -LiteralPath $frontendStamp -Value $frontendHash -Encoding UTF8
    }
    $script:Npm = $npmCommand.Source
}

function Read-ServiceRecord {
    param([ValidateSet('backend', 'frontend', 'test')][string]$Name)
    $path = Join-Path $script:PidDir "$Name.json"
    if (-not (Test-Path -LiteralPath $path)) { return $null }
    try { return Get-Content -LiteralPath $path -Raw | ConvertFrom-Json }
    catch { Write-Host "[warn] Invalid service record: $path"; return $null }
}

function Get-ManagedProcess {
    param([ValidateSet('backend', 'frontend', 'test')][string]$Name)
    $record = Read-ServiceRecord $Name
    if (-not $record) { return $null }
    try {
        $process = Get-Process -Id $record.processId -ErrorAction Stop
        if ($process.StartTime.ToUniversalTime().Ticks.ToString() -eq $record.startTicks -and $process.Path -eq $record.executable) { return $process }
    } catch { return $null }
    return $null
}

function Test-PortInUse {
    param([int]$Port)
    $listener = New-Object System.Net.Sockets.TcpListener([System.Net.IPAddress]::Loopback, $Port)
    try { $listener.Start(); return $false }
    catch { return $true }
    finally { $listener.Stop() }
}

function Assert-ServicePort {
    param([ValidateSet('backend', 'frontend')][string]$Name, [int]$Port)
    if (Get-ManagedProcess $Name) {
        $record = Read-ServiceRecord $Name
        if ($record.port -ne $Port) { throw "$Name is already running on port $($record.port). Run .\start.cmd -Stop before changing ports." }
    } elseif (Test-PortInUse $Port) {
        throw "Port $Port is occupied by another process (possibly WSL). Stop that service or select another port. No unrelated process has been stopped."
    }
}

function ConvertTo-ProcessArgument {
    param([string]$Value)
    if ($Value.Length -gt 0 -and $Value -notmatch '[\s"]') { return $Value }
    # Windows CommandLineToArgvW escaping, including backslashes before a quote.
    $escaped = [regex]::Replace($Value, '(\\*)"', '$1$1\"')
    $escaped = [regex]::Replace($escaped, '(\\+)$', '$1$1')
    return '"' + $escaped + '"'
}

function Start-ManagedService {
    param([ValidateSet('backend', 'frontend', 'test')][string]$Name, [string]$Executable, [string[]]$Arguments, [string]$WorkingDirectory, [int]$Port)
    New-Item -ItemType Directory -Path $script:PidDir, $script:LogDir -Force | Out-Null
    $argumentLine = ($Arguments | ForEach-Object { ConvertTo-ProcessArgument $_ }) -join ' '
    $process = Start-Process -FilePath $Executable -ArgumentList $argumentLine -WorkingDirectory $WorkingDirectory -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $script:LogDir "$Name.log") -RedirectStandardError (Join-Path $script:LogDir "$Name.error.log")
    try {
        # Start-Process can return before the image path is available on Windows.
        # Take the identity from a fresh process snapshot, just as the reader does.
        $deadline = [DateTime]::UtcNow.AddSeconds(5)
        do {
            if ($process.HasExited) { throw "$Name exited during startup. Check .logs\windows\$Name.error.log." }
            $identity = Get-Process -Id $process.Id -ErrorAction Stop
            if ($identity.Path) { break }
            Start-Sleep -Milliseconds 50
        } while ([DateTime]::UtcNow -lt $deadline)
        if (-not $identity.Path) { throw "Could not read $Name process identity." }
        @{ processId = $identity.Id; startTicks = $identity.StartTime.ToUniversalTime().Ticks.ToString(); executable = $identity.Path; port = $Port; apiUrl = $env:LOGIC_LAB_API_URL } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $script:PidDir "$Name.json") -Encoding UTF8
    } catch {
        if (-not $process.HasExited) { Stop-Process -InputObject $process -Force }
        throw
    }
    Write-Host "[start] $Name (PID $($process.Id), port $Port)"
}

function Stop-ManagedService {
    param([ValidateSet('backend', 'frontend', 'test')][string]$Name)
    $process = Get-ManagedProcess $Name
    if ($process) { Stop-Process -InputObject $process -Force -ErrorAction SilentlyContinue; Write-Host "[stop] $Name" }
    else { Write-Host "[stop] $Name is not running as a managed service" }
    $path = Join-Path $script:PidDir "$Name.json"
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path }
}

function Invoke-LocalHttp {
    param([string]$Url, [switch]$AsJson)
    Add-Type -AssemblyName System.Net.Http
    $handler = New-Object System.Net.Http.HttpClientHandler
    $handler.UseProxy = $false
    $client = New-Object System.Net.Http.HttpClient($handler)
    $client.Timeout = [TimeSpan]::FromSeconds(2)
    $response = $null
    try {
        $response = $client.GetAsync($Url).GetAwaiter().GetResult()
        $null = $response.EnsureSuccessStatusCode()
        $body = $response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
        if ($AsJson) { return $body | ConvertFrom-Json }
        return $body
    } finally {
        if ($response) { $response.Dispose() }
        $client.Dispose()
        $handler.Dispose()
    }
}

function Wait-ServiceReady {
    param([string]$Url, [ValidateSet('backend', 'frontend')][string]$Name, [switch]$CoreRequired)
    $deadline = [DateTime]::UtcNow.AddSeconds(30)
    $lastError = 'No response received'
    while ([DateTime]::UtcNow -lt $deadline) {
        if (-not (Get-ManagedProcess $Name)) { throw "$Name exited. Check .logs\windows\$Name.error.log." }
        try {
            if ($CoreRequired) {
                $health = Invoke-LocalHttp $Url -AsJson
                if (-not $health.sim_core_available) { throw 'Simulation core unavailable' }
            } else { $null = Invoke-LocalHttp $Url }
            Write-Host "[ready] $Name"
            return
        } catch { $lastError = $_.Exception.Message; Start-Sleep -Milliseconds 300 }
    }
    throw "$Name did not become ready: $lastError. Check .logs\windows\$Name.error.log."
}

function Show-WindowsServiceStatus {
    param([int]$BackendPort, [int]$FrontendPort)
    foreach ($entry in @(@{ Name = 'backend'; Port = $BackendPort }, @{ Name = 'frontend'; Port = $FrontendPort })) {
        $process = Get-ManagedProcess $entry.Name
        if ($process) {
            $record = Read-ServiceRecord $entry.Name
            Write-Host "$($entry.Name): RUNNING (managed PID $($process.Id), port $($record.port))"
        } elseif (Test-PortInUse $entry.Port) { Write-Host "$($entry.Name): port $($entry.Port) occupied by an external process" }
        else { Write-Host "$($entry.Name): STOPPED" }
    }
}
