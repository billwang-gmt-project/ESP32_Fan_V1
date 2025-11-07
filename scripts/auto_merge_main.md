# Auto-merge helper: how to run `auto_merge_main.ps1`

This document explains how to run the PowerShell helper script `scripts/auto_merge_main.ps1` that checks out a local branch (default `main`), fetches from a remote (default `origin`), and merges a specified remote branch into the local branch. It includes examples, parameters, safety tips, and troubleshooting steps.

## File location

- `scripts/auto_merge_main.ps1`
- This memo: `scripts/auto_merge_main.md`

## Purpose

Use the script when you want to automate a safe local merge of a remote branch into your local branch. The script:

- Verifies git is available and that you're inside a git repository
- Optionally stashes uncommitted changes (default behavior)
- Checks out the local branch (default `main`)
- Fetches from the remote (default `origin`)
- Merges the specified remote branch into the local branch
- Pops the stash if one was created

## Prerequisites

- Windows with PowerShell (the script is written for PowerShell)
- `git` available on PATH
- Run the script from the repository root (where `.git` exists)
- If your system blocks unsigned scripts, you may need to adjust ExecutionPolicy (see examples)

## Basic usage

Open PowerShell in the repository root and run the script. The script has sensible defaults and will merge the branch you requested in the original task by default.

```powershell
# Run with defaults (Remote=origin, RemoteBranch=claude/clone-arduino-webserver-011CUsix8cqsPbNXCgK5kEMZ, LocalBranch=main)
.\scripts\auto_merge_main.ps1
```

## Common options

- -Remote <name>
  - Remote name to fetch from (default: `origin`).
- -RemoteBranch <branch>
  - Remote branch to merge (default: `claude/clone-arduino-webserver-011CUsix8cqsPbNXCgK5kEMZ`).
- -LocalBranch <branch>
  - Local branch to check out and merge into (default: `main`).
- -AutoStash:$false
  - Disable automatic stashing. If you pass `-AutoStash:$false` the script will fail when there are uncommitted changes.

Examples:

```powershell
# Merge a different remote branch into main
.\scripts\auto_merge_main.ps1 -RemoteBranch 'feature/some-feature'

# Specify a different local branch
.\scripts\auto_merge_main.ps1 -LocalBranch 'develop' -RemoteBranch 'origin/develop'

# Disable automatic stashing (script will error if working tree dirty)
.\scripts\auto_merge_main.ps1 -AutoStash:$false
```

## ExecutionPolicy note

If your environment blocks script execution, run PowerShell with an ExecutionPolicy override for this single invocation:

```powershell
PowerShell -ExecutionPolicy Bypass -File .\scripts\auto_merge_main.ps1
```

## Safety tips

- The script stashes uncommitted changes (including untracked files) by default and attempts to pop the stash after a successful merge.
- If you want an extra safety layer, create a backup branch before running the script:

```powershell
git branch main-pre-merge-$(Get-Date -Format yyyyMMdd-HHmmss)
.\scripts\auto_merge_main.ps1
```

- If the merge produces conflicts, the script will exit with a non-zero error and you must resolve conflicts manually. After resolving, run `git commit` to finish the merge.

## Troubleshooting

- "git is not available in PATH": install Git for Windows and ensure `git` is on PATH.
- "Current directory is not inside a git repository": cd to the repo root (where `.git` is) before running.
- Uncommitted changes detected and you didn't allow stashing: re-run with `-AutoStash` or commit your changes.
- Merge conflicts: run `git status` to see conflict files, resolve them, then `git add <file>` and `git commit`. If you want to abort the merge and return to the pre-merge state, run `git merge --abort`.
- Stash pop failed: run `git stash list` and inspect/restore manually with `git stash apply` or `git stash pop`.

## Exit codes

- `0` — success
- non-zero — failure (messages printed to stderr). The script prints hints to fix common issues.

## CI / automation notes

- The script is intended for local use. If you plan to call it from CI, ensure the runner has git credentials configured (e.g., via SSH key or token) and that running scripts is allowed in your environment. For CI, consider replacing interactive flows (stashing, stash pop) with an explicit workflow that preserves or commits workspace changes programmatically.

## Next steps / enhancements (optional)

- Add a `--dry-run` mode that fetches and prints what would be merged without performing the merge.
- Add an option to create an automatic backup branch before merging.
- Add a log file to persist operation output for auditing.

---

If you'd like, I can add a `--backup` flag to create `main-pre-merge-<timestamp>` automatically before merging, or implement a dry-run mode. Tell me which enhancement you prefer and I will implement it next.
