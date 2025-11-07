param(
    [string]$Remote = 'origin',
    [string]$RemoteBranch = 'claude/clone-arduino-webserver-011CUsix8cqsPbNXCgK5kEMZ',
    [string]$LocalBranch = 'main',
    [switch]$AutoStash
)

if (-not $PSBoundParameters.ContainsKey('AutoStash')) {
    # Default to stashing changes to allow non-interactive runs
    $AutoStash = $true
}

function Run-Git {
    param([string]$Args)
    Write-Host "git $Args"
    git $Args
    if ($LASTEXITCODE -ne 0) {
        throw "git $Args failed with exit code $LASTEXITCODE"
    }
}

try {
    # Ensure git exists
    $gitVersion = & git --version 2>$null
    if ($LASTEXITCODE -ne 0) { throw 'git is not available in PATH.' }

    # Ensure we're inside a git repo
    $inside = (& git rev-parse --is-inside-work-tree) -eq 'true'
    if (-not $inside) { throw 'Current directory is not inside a git repository.' }

    # Check for uncommitted changes
    $status = & git status --porcelain
    $stashCreated = $false
    if ($status) {
        if ($AutoStash) {
            Write-Host 'Uncommitted changes detected — stashing (including untracked files)...'
            & git stash push -u -m "auto-merge-stash-$(Get-Date -Format o)" | Out-Null
            if ($LASTEXITCODE -ne 0) { throw 'git stash failed' }
            $stashCreated = $true
        } else {
            throw 'Uncommitted changes present. Rerun with -AutoStash to stash automatically.'
        }
    }

    # Checkout local branch
    Write-Host "Checking out local branch '$LocalBranch'..."
    Run-Git "checkout $LocalBranch"

    # Fetch from remote
    Write-Host "Fetching from remote '$Remote'..."
    Run-Git "fetch $Remote"

    # Merge the remote branch
    $remoteRef = "$Remote/$RemoteBranch"
    Write-Host "Merging '$remoteRef' into '$LocalBranch'..."

    # Attempt a fast-forward / merge. Use --no-edit to avoid interactive editor on merge commit message.
    & git merge --no-edit $remoteRef
    if ($LASTEXITCODE -ne 0) {
        throw "Merge failed (exit code $LASTEXITCODE). Resolve conflicts manually and run 'git merge --abort' if needed."
    }

    Write-Host 'Merge completed successfully.' -ForegroundColor Green

    if ($stashCreated) {
        Write-Host 'Popping previously created stash...'
        & git stash pop
        if ($LASTEXITCODE -ne 0) {
            Write-Warning 'git stash pop failed. You may need to restore the stash manually (run `git stash list`).'
        } else {
            Write-Host 'Stash popped.'
        }
    }

    Write-Host 'All done.' -ForegroundColor Cyan
    exit 0
}
catch {
    Write-Error "Error: $_"
    Write-Host 'Tip: check for merge conflicts and uncommitted changes, then re-run the script.'
    exit 1
}
