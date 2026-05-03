$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$defaultGodotExe = "D:\program\tools\godot\Godot_v4.6.1-stable_win64_console.exe"
$godotExe = if ($env:GODOT_EXE) { $env:GODOT_EXE } else { $defaultGodotExe }
$logDir = Join-Path $repoRoot ".godot\agent-logs"
$tests = @(
    @{
        Name = "board rules"
        Script = "res://scripts/tests/board_rules_smoke_test.gd"
        Log = "board_rules_smoke_test.log"
    },
    @{
        Name = "board turns"
        Script = "res://scripts/tests/board_turns_smoke_test.gd"
        Log = "board_turns_smoke_test.log"
    },
    @{
        Name = "turn order"
        Script = "res://scripts/tests/turn_order_smoke_test.gd"
        Log = "turn_order_smoke_test.log"
    },
    @{
        Name = "scoring"
        Script = "res://scripts/tests/scoring_smoke_test.gd"
        Log = "scoring_smoke_test.log"
    },
    @{
        Name = "reset cancellation"
        Script = "res://scripts/tests/reset_cancellation_smoke_test.gd"
        Log = "reset_cancellation_smoke_test.log"
    },
    @{
        Name = "animation"
        Script = "res://scripts/tests/animation_smoke_test.gd"
        Log = "animation_smoke_test.log"
    }
)

if (-not (Test-Path -LiteralPath $godotExe)) {
    throw "Godot executable not found: $godotExe. Set GODOT_EXE to override."
}

New-Item -ItemType Directory -Force -Path $logDir | Out-Null

foreach ($test in $tests) {
    $logFile = Join-Path $logDir $test.Log
    Write-Host "Running $($test.Name) smoke test..."

    & $godotExe `
        --headless `
        --log-file $logFile `
        --path $repoRoot `
        --script $test.Script

    if ($LASTEXITCODE -ne 0) {
        throw "Godot $($test.Name) smoke test failed with exit code $LASTEXITCODE."
    }
}
