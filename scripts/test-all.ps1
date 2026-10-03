[CmdletBinding()]
param([string]$Python = '', [string]$Generator = 'Visual Studio 17 2022')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'windows-common.ps1')
$previousBuild = $env:LOGIC_SIM_BUILD_DIR
$previousPath = $env:PYTHONPATH
$previousEncoding = $env:PYTHONIOENCODING
try {
    if ((Get-ManagedProcess 'backend') -or (Get-ManagedProcess 'frontend')) { throw 'Stop managed services with .\start.cmd -Stop before running the full build/test suite.' }
    Initialize-WindowsEnvironment -Python $Python -Generator $Generator -BuildTests
    $env:LOGIC_SIM_BUILD_DIR = $script:ModuleDir
    $env:PYTHONPATH = "$script:ModuleDir;$script:ProjectRoot\backend"
    $env:PYTHONIOENCODING = 'utf-8'
    $ctest = Join-Path (Split-Path $script:CMake -Parent) 'ctest.exe'
    Invoke-Checked $ctest @('--test-dir', $script:BuildDir, '-C', 'Release', '--output-on-failure')
    # The smoke script intentionally skips when the module is missing: require import first.
    Invoke-Checked $script:VenvPython @('-c', 'import logic_sim; print(logic_sim.__file__)')
    Invoke-Checked $script:VenvPython @((Join-Path $script:ProjectRoot 'bindings/python/tests/test_python_binding.py'))
    Invoke-Checked $script:VenvPython @('-m', 'pytest', 'tests', '-q') (Join-Path $script:ProjectRoot 'backend')
    $frontend = Join-Path $script:ProjectRoot 'frontend'
    Invoke-Checked $script:Npm @('run', 'build') $frontend
    Invoke-Checked $script:Npm @('run', 'lint') $frontend
    Invoke-Checked $script:Npm @('exec', '--', 'vitest', 'run') $frontend
    & (Join-Path $PSScriptRoot 'test-windows.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Windows launcher regression checks failed.' }
    Write-Host 'All native Windows tests passed.'
    exit 0
} catch {
    Write-Host "[ERROR] $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    $env:LOGIC_SIM_BUILD_DIR = $previousBuild
    $env:PYTHONPATH = $previousPath
    $env:PYTHONIOENCODING = $previousEncoding
}
