# Dependency-free regression tests for quoting, ownership, and occupied ports.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'windows-common.ps1')
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('logic-lab windows-' + [guid]::NewGuid().ToString('N'))
$testProcess = $null
$listener = $null
function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
try {
    foreach ($file in @('start.ps1', 'scripts/windows-common.ps1', 'scripts/test-all.ps1', 'scripts/test-windows.ps1', 'scripts/package-windows.ps1', 'scripts/test-installer.ps1')) {
        $tokens = $null; $parseErrors = $null
        $null = [System.Management.Automation.Language.Parser]::ParseFile((Join-Path $script:ProjectRoot $file), [ref]$tokens, [ref]$parseErrors)
        Assert-True ($parseErrors.Count -eq 0) "PowerShell parse error: $file"
    }
    $script:PidDir = Join-Path $testRoot 'pids'
    $script:LogDir = Join-Path $testRoot 'logs'
    New-Item -ItemType Directory -Path $testRoot -Force | Out-Null
    $unicodeDirectory = Join-Path $testRoot ('fixture-' + [char]0x7535 + [char]0x8DEF)
    New-Item -ItemType Directory -Path $unicodeDirectory -Force | Out-Null
    $node = (Get-Command node.exe -ErrorAction Stop).Source
    $batchPath = Join-Path $unicodeDirectory 'command with spaces.cmd'
    Set-Content -LiteralPath $batchPath -Value "@echo off`r`necho wrapper-ok`r`nexit /b 0" -Encoding ASCII
    $batchResult = [string](Invoke-Checked $batchPath @() $unicodeDirectory -Capture)
    Assert-True ($batchResult.Trim() -eq 'wrapper-ok') 'Batch executable path did not round-trip.'
    $output = Join-Path $unicodeDirectory 'arguments.json'
    $values = @('path with spaces', ('Unicode-' + [char]0x7535 + [char]0x8DEF), 'embedded"quote', 'trailing slash\', '')
    $checked = (Invoke-Checked $node (@('-e', 'console.log(JSON.stringify(process.argv.slice(1)))') + $values) $unicodeDirectory -Capture) | ConvertFrom-Json
    Assert-True ($checked.Count -eq $values.Count) 'Checked command argument count changed.'
    for ($i = 0; $i -lt $values.Count; $i++) { Assert-True ($checked[$i] -ceq $values[$i]) "Checked command argument $i did not round-trip." }
    $failed = $false
    try { Invoke-Checked $node @('-e', 'process.exit(7)') $unicodeDirectory -Capture }
    catch { $failed = $_.Exception.Message -like 'Command failed (7):*' }
    Assert-True $failed 'A failed native command was reported as success.'
    $program = 'const fs=require("node:fs");const server=require("node:http").createServer((req,res)=>{res.setHeader("Content-Type","application/json");res.end(JSON.stringify({sim_core_available:true}))});server.listen(0,"127.0.0.1",()=>{fs.writeFileSync(process.argv[1]+".port",String(server.address().port));fs.writeFileSync(process.argv[1],JSON.stringify(process.argv.slice(2)))});'
    Start-ManagedService 'test' $node (@('-e', $program, $output) + $values) $unicodeDirectory 0
    $deadline = [DateTime]::UtcNow.AddSeconds(5)
    while (-not (Test-Path -LiteralPath $output) -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 50 }
    Assert-True (Test-Path -LiteralPath $output) 'Child process did not receive arguments.'
    $actual = Get-Content -LiteralPath $output -Raw -Encoding UTF8 | ConvertFrom-Json
    Assert-True ($actual.Count -eq $values.Count) "Argument count changed: $(Get-Content -LiteralPath $output -Raw -Encoding UTF8)"
    for ($i = 0; $i -lt $values.Count; $i++) { Assert-True ($actual[$i] -ceq $values[$i]) "Argument $i did not round-trip." }
    $childPort = [int](Get-Content -LiteralPath ($output + '.port') -Raw)
    $health = Invoke-LocalHttp "http://127.0.0.1:$childPort/api/health" -AsJson
    Assert-True $health.sim_core_available 'Local health check failed.'
    $testProcess = Get-ManagedProcess 'test'
    Assert-True ($null -ne $testProcess) 'Live managed process was not recognized.'
    $recordPath = Join-Path $script:PidDir 'test.json'
    $originalRecord = Get-Content -LiteralPath $recordPath -Raw
    $staleRecord = $originalRecord | ConvertFrom-Json
    $staleRecord.startTicks = '0'
    $staleRecord | ConvertTo-Json | Set-Content -LiteralPath $recordPath -Encoding UTF8
    Assert-True ($null -eq (Get-ManagedProcess 'test')) 'Stale PID was incorrectly treated as owned.'
    Stop-ManagedService 'test'
    $testProcess.Refresh()
    Assert-True (-not $testProcess.HasExited) 'Stop killed a process with a mismatched identity.'
    Set-Content -LiteralPath $recordPath -Value $originalRecord -Encoding UTF8
    Stop-ManagedService 'test'
    $null = $testProcess.WaitForExit(5000)
    Assert-True $testProcess.HasExited 'Owned process did not stop.'
    Assert-True (-not (Test-Path -LiteralPath $recordPath)) 'Process record was not cleaned up.'
    $listener = New-Object System.Net.Sockets.TcpListener([System.Net.IPAddress]::Loopback, 0)
    $listener.Start()
    $port = $listener.LocalEndpoint.Port
    $rejected = $false
    try { Assert-ServicePort 'backend' $port } catch { $rejected = $_.Exception.Message -like '*occupied*' }
    Assert-True $rejected 'External port collision was not rejected.'
    Assert-True $listener.Server.IsBound 'An external listener was affected.'
    Write-Host '[PASS] Windows argument quoting, Unicode paths, exit codes, local HTTP, managed stop, stale PID protection, port collision, and script syntax'
    exit 0
} catch {
    Write-Host "[FAIL] $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    if ($listener) { $listener.Stop() }
    if ($testProcess) {
        $testProcess.Refresh()
        if (-not $testProcess.HasExited) { Stop-Process -InputObject $testProcess -Force }
    } else { Stop-ManagedService 'test' }
    # Delete only the uniquely named fixture directory inside the OS temp directory.
    $resolvedTest = [IO.Path]::GetFullPath($testRoot)
    $resolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if ($resolvedTest.StartsWith($resolvedTemp, [StringComparison]::OrdinalIgnoreCase) -and (Split-Path $resolvedTest -Leaf).StartsWith('logic-lab windows-') -and (Test-Path -LiteralPath $resolvedTest)) {
        Remove-Item -LiteralPath $resolvedTest -Recurse -Force
    }
}
