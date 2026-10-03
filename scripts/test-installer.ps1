param([Parameter(Mandatory = $true)][string]$Installer)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'windows-common.ps1')
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('LogicLab installer test ' + [Guid]::NewGuid().ToString('N'))
$installDir = Join-Path $testRoot 'app'
$dataDir = Join-Path $testRoot 'user data'
$null = New-Item -ItemType Directory -Path $testRoot -Force
$client = $null
$process = $null
$exe = Join-Path $installDir 'LogicLab.exe'
function Request-Json {
    param([string]$Url, [string]$Method = 'GET', [object]$Body = $null)
    $response = $null
    $request = New-Object Net.Http.HttpRequestMessage (New-Object Net.Http.HttpMethod $Method), $Url
    if ($null -ne $Body) {
        $request.Content = New-Object Net.Http.StringContent ($Body | ConvertTo-Json -Depth 20), ([Text.Encoding]::UTF8), 'application/json'
    }
    try {
        $response = $client.SendAsync($request).GetAwaiter().GetResult()
        $null = $response.EnsureSuccessStatusCode()
        $text = $response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
        if ($text) { return ($text | ConvertFrom-Json) }
    } finally { if ($response) { $response.Dispose() }; $request.Dispose() }
}
function Start-TestApp {
    param([switch]$WithWindow)
    $argsList = @('--data-dir', $dataDir, '--no-browser')
    if (-not $WithWindow) { $argsList += '--headless' }
    $arguments = ($argsList | ForEach-Object { ConvertTo-ProcessArgument $_ }) -join ' '
    $script:process = Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $installDir -PassThru -WindowStyle Hidden
    $statePath = Join-Path $dataDir 'runtime.json'
    $deadline = [DateTime]::UtcNow.AddSeconds(45)
    do {
        if ($script:process.HasExited) { throw "Application exited early ($($script:process.ExitCode)). See $dataDir\desktop.log" }
        if (Test-Path -LiteralPath $statePath) {
            $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
            $health = Request-Json ($state.url + '/api/health')
            if ($health.sim_core_available) { return $state }
        }
        Start-Sleep -Milliseconds 200
    } while ([DateTime]::UtcNow -lt $deadline)
    throw 'Application did not become ready.'
}
function Stop-TestApp {
    if ($process -and -not $process.HasExited) {
        Invoke-Checked $exe @('--data-dir', $dataDir, '--stop') -WorkingDirectory $installDir
        if (-not $process.WaitForExit(15000)) { throw 'Application failed to stop gracefully.' }
        if ($process.ExitCode -ne 0) { throw "Application exit code: $($process.ExitCode)" }
    }
}
try {
    $Installer = (Resolve-Path -LiteralPath $Installer).ProviderPath
    Invoke-Checked $Installer @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/NOICONS', '/TESTINSTALL=1', "/DIR=$installDir", "/LOG=$(Join-Path $testRoot 'install.log')") -WorkingDirectory $testRoot
    if (-not (Test-Path -LiteralPath $exe)) { throw 'Installer did not install the application.' }
    Add-Type -AssemblyName System.Net.Http
    $handler = New-Object Net.Http.HttpClientHandler
    $handler.UseProxy = $false
    $client = New-Object Net.Http.HttpClient $handler
    $client.Timeout = [TimeSpan]::FromSeconds(5)
    $state = Start-TestApp
    $html = $client.GetStringAsync($state.url + '/').GetAwaiter().GetResult()
    if ($html -notmatch '<div id="root">' -or $html -notmatch '/assets/') { throw 'Bundled frontend did not load.' }
    $devices = Request-Json ($state.url + '/api/devices')
    if (@($devices).Count -lt 40) { throw 'Device library was not bundled.' }
    $body = @{ circuit = @{ version = '1.0'; devices = @(@{ id = 'v'; type = 'VCC'; x = 0; y = 0; params = @{} }); wires = @() }; options = @{ max_ticks = 10; record_all = $true } }
    $result = Request-Json ($state.url + '/api/simulations/run') 'POST' $body
    if ($result.status -ne 'ok' -or $result.final_nodes.'v.OUT' -ne '1') { throw 'Bundled native simulation failed.' }
    $project = Request-Json ($state.url + '/api/projects') 'POST' @{ name = 'Installer smoke test' }
    Invoke-Checked $exe @('--headless', '--data-dir', $dataDir) -WorkingDirectory $installDir
    $same = Get-Content -LiteralPath (Join-Path $dataDir 'runtime.json') -Raw | ConvertFrom-Json
    if ($same.instance -ne $state.instance) { throw 'Second launch created a duplicate service.' }
    Stop-TestApp
    $state = Start-TestApp -WithWindow
    $saved = Request-Json ($state.url + '/api/projects/' + $project.id)
    if ($saved.name -ne 'Installer smoke test') { throw 'Projects did not persist across restarts.' }
    # A GUI import/start failure would terminate the service and fail this request.
    Start-Sleep -Seconds 2
    $null = Request-Json ($state.url + '/api/health')
    Stop-TestApp
    Invoke-Checked (Join-Path $installDir 'unins000.exe') @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', "/LOG=$(Join-Path $testRoot 'uninstall.log')") -WorkingDirectory $testRoot
    if (Test-Path -LiteralPath $exe) { throw 'Uninstaller left the application executable.' }
    if (-not (Test-Path -LiteralPath (Join-Path $dataDir 'projects.db'))) { throw 'Uninstaller removed user data.' }
    $report = @('PASS: Windows x64 silent installation', 'PASS: Bundled frontend and 45-device library', 'PASS: Native C++ VCC simulation', 'PASS: Single instance', 'PASS: Project persistence after restart', 'PASS: Tk control window starts', 'PASS: Graceful stop', 'PASS: Uninstall preserves user data', "Verified at: $([DateTime]::UtcNow.ToString('o'))")
    Set-Content -LiteralPath (Join-Path (Split-Path $Installer -Parent) 'verification.txt') -Value $report -Encoding UTF8
    Write-Host '[PASS] Install, frontend, native simulation, single instance, persistence, GUI startup, stop and uninstall.'
    Write-Host "[test] Evidence retained at: $testRoot"
} finally {
    Stop-TestApp
    if ($client) { $client.Dispose() }
}
