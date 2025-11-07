<#
D:\github\ESP32\fw\Motor_Control_V1\scripts\run.ps1

PowerShell wrapper to run PlatformIO tasks. Prints debug info (script path, environment)
then runs the same sequence: clean, build, upload, uploadfs.

Usage:
  .\run.ps1            # from scripts folder
  ..\scripts\run.ps1  # from project root
  run.ps1 -NoPause      # suppress final pause
#>
param(
    [switch] $NoPause
)

function Write-DebugInfo {
    Write-Host "--- run.ps1 debug info ---"
    Write-Host "Script path: $($PSCommandPath)"
    Write-Host "Script directory: $PSScriptRoot"
    Write-Host "Working directory: $(Get-Location)"
    Write-Host "PowerShell Version: $($PSVersionTable.PSVersion)"
    Write-Host "OS: $([System.Environment]::OSVersion)"
    Write-Host "Environment PATH (first 2 entries):"
    $env:PATH -split ';' | Select-Object -First 5 | ForEach-Object { Write-Host "  $_" }
    Write-Host "Where 'pio' is found (if any):"
    try {
        Get-Command pio -ErrorAction Stop | ForEach-Object { Write-Host "  $($_.Source) -> $($_.Definition)" }
    } catch {
        # fallback to where.exe
        $where = & where.exe pio 2>$null
        if ($where) { $where -split "\r?\n" | ForEach-Object { Write-Host "  $_" } } else { Write-Host "  pio not found in PATH" }
    }
    Write-Host "Arguments: $($args -join ' ')"
    Write-Host "---------------------------"
}

function Run-Check($command) {
    Write-Host "-> $command"
    # run via cmd to preserve prior behavior and error codes
    & cmd /c $command
    $ec = $LASTEXITCODE
    if ($ec -ne 0) {
        Write-Error "Command failed: $command (exit $ec)"
        exit $ec
    }
}

# Print debug info
Write-DebugInfo

# Execute the PlatformIO steps
Run-Check "pio run -t clean"
Run-Check "pio run"
Run-Check "pio run --target upload"
Run-Check "pio run --target uploadfs"

Write-Host "All PlatformIO tasks completed successfully."
if (-not $NoPause) { Read-Host -Prompt "Press Enter to close" }
