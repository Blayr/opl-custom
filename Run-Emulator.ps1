param(
    [string]$Emulator = 'C:\Users\Blair\Downloads\pcsx2-2.8.2\pcsx2-qt.exe'
)
$ErrorActionPreference = 'Stop'
$elf = Join-Path $PSScriptRoot 'opl.elf'
$profile = Join-Path $PSScriptRoot '.tools\pcsx2'
if (!(Test-Path -LiteralPath $Emulator)) { throw "PCSX2 not found: $Emulator" }
if (!(Test-Path -LiteralPath $elf)) { throw 'Run Build-Custom.ps1 first.' }
if (!(Test-Path -LiteralPath "$profile\PCSX2\inis\PCSX2.ini")) {
    throw 'The local development profile has not been configured.'
}
Start-Process -FilePath $Emulator -ArgumentList '-datapath', "`"$profile`"", '-elf', "`"$elf`""
