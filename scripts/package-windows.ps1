param(
    [string]$Python = '',
    [string]$Generator = 'Visual Studio 17 2022',
    [string]$InnoSetup = '',
    [string]$Version = ''
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'windows-common.ps1')
$previousBuildDir = $env:LOGIC_SIM_BUILD_DIR
$previousLicenseDir = $env:LOGIC_LAB_LICENSE_DIR
try {
    Initialize-WindowsEnvironment -Python $Python -Generator $Generator
    $runtimeVersion = [string](Invoke-Checked $script:VenvPython @('-c', 'from backend.app.runtime import VERSION; print(VERSION)') -Capture | Select-Object -Last 1)
    if ($Version -and $Version -ne $runtimeVersion) { throw 'Installer version must match backend/app/runtime.py.' }
    $Version = $runtimeVersion
    if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw 'Version must be major.minor.patch.' }
    Invoke-Checked $script:VenvPython @('-m', 'pip', 'install', '-r', (Join-Path $script:ProjectRoot 'packaging/requirements.txt'))
    Invoke-Checked (Get-Command npm.cmd).Source @('run', 'build') -WorkingDirectory (Join-Path $script:ProjectRoot 'frontend')
    # Fail rather than ship a package with a missing/incompatible native core.
    $env:LOGIC_SIM_BUILD_DIR = $script:ModuleDir
    Invoke-Checked $script:VenvPython @('-c', 'import os,sys; sys.path.insert(0,os.environ["LOGIC_SIM_BUILD_DIR"]); import logic_sim; import tkinter; print(logic_sim.__file__)')
    $env:LOGIC_LAB_LICENSE_DIR = [string](Invoke-Checked $script:VenvPython @((Join-Path $script:ProjectRoot 'packaging/collect_licenses.py'), $script:ProjectRoot) -Capture | Select-Object -Last 1)
    $dist = Join-Path $script:ProjectRoot 'dist'
    Invoke-Checked $script:VenvPython @('-m', 'PyInstaller', '--noconfirm', '--clean', '--distpath', $dist, '--workpath', (Join-Path $script:BuildDir 'pyinstaller'), (Join-Path $script:ProjectRoot 'packaging/logiclab.spec'))
    if (-not $InnoSetup) {
        $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
        if ($command) { $InnoSetup = $command.Source }
        foreach ($base in @([Environment]::GetFolderPath('ProgramFilesX86'), [Environment]::GetFolderPath('ProgramFiles'), $env:LOCALAPPDATA)) {
            $candidate = Join-Path $base 'Inno Setup 6/ISCC.exe'
            if (-not $InnoSetup -and (Test-Path -LiteralPath $candidate)) { $InnoSetup = $candidate }
        }
    }
    if (-not $InnoSetup -or -not (Test-Path -LiteralPath $InnoSetup)) { throw 'Install Inno Setup 6, or pass -InnoSetup C:\tools\InnoSetup\ISCC.exe.' }
    $output = Join-Path $dist 'installers'
    $null = New-Item -ItemType Directory -Path $output -Force
    Invoke-Checked $InnoSetup @("/DAppVersion=$Version", "/DBundleDir=$(Join-Path $dist 'LogicLab')", "/DOutputDir=$output", (Join-Path $script:ProjectRoot 'packaging/installer.iss'))
    $installer = Join-Path $output "LogicLab-$Version-Windows-x64-Setup.exe"
    $hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
    Set-Content -LiteralPath (Join-Path $output 'SHA256SUMS.txt') -Value "$hash  $([IO.Path]::GetFileName($installer))" -Encoding ASCII
    Copy-Item -LiteralPath (Join-Path $script:ProjectRoot 'docs/安装包说明.md') -Destination $output
    $dependencies = Invoke-Checked $script:VenvPython @('-m', 'pip', 'freeze') -Capture
    Set-Content -LiteralPath (Join-Path $output 'build-dependencies.txt') -Value $dependencies -Encoding UTF8
    Write-Host "[package] Installer: $installer"
    Write-Host "[package] SHA256: $hash"
} catch {
    Write-Host "[ERROR] $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    $env:LOGIC_SIM_BUILD_DIR = $previousBuildDir
    $env:LOGIC_LAB_LICENSE_DIR = $previousLicenseDir
}
