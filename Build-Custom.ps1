param([int]$Jobs = 4)
$ErrorActionPreference = 'Stop'
$project = $PSScriptRoot
$image = 'ghcr.io/ps2homebrew/ps2homebrew@sha256:f92b526029a743bc30fb47d0af096f67d7e4e97e4019313f65508f6f6cdaaa91'
if ($Jobs -lt 1 -or $Jobs -gt 32) { throw 'Jobs must be between 1 and 32.' }
& docker info --format '{{.ServerVersion}}'
if ($LASTEXITCODE -ne 0) { throw 'Start Docker Desktop with its Linux engine first.' }
& docker run --rm -v "${project}:/src" -w /src $image sh -c 'python3 tests/run_favorites.py'
if ($LASTEXITCODE -ne 0) { throw 'Favorites tests failed.' }
& docker run --rm -v "${project}:/src" -w /src $image sh -c 'python3 tests/run_prefetch.py'
if ($LASTEXITCODE -ne 0) { throw 'Artwork prefetch tests failed.' }
$buildCommand = "git config --global --add safe.directory /src; make -j$Jobs all LOCALVERSION=custom > build-custom.log 2>&1"
& docker run --rm -v "${project}:/src" -w /src $image sh -c $buildCommand
if ($LASTEXITCODE -ne 0) { throw "Build failed. See $project\build-custom.log" }
$destination = Join-Path $project 'dist'
New-Item -ItemType Directory -Path $destination -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $project 'OPNPS2LD.ELF') -Destination (Join-Path $destination 'OPL_CUSTOM.ELF')
Copy-Item -LiteralPath (Join-Path $project 'CUSTOM.md') -Destination $destination
Copy-Item -LiteralPath (Join-Path $project 'LICENSE') -Destination $destination
$hash = (Get-FileHash -LiteralPath (Join-Path $destination 'OPL_CUSTOM.ELF') -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  OPL_CUSTOM.ELF" | Set-Content -LiteralPath (Join-Path $destination 'SHA256SUMS.txt') -Encoding ascii
Write-Host "Built $destination\OPL_CUSTOM.ELF"
