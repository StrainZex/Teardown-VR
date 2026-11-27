param([ValidateSet('Probe','Disable')][string]$Mode='Probe')
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'Resolve-VRBuild.ps1')
$games=@(Get-Process teardown -ErrorAction SilentlyContinue)
if($games.Count -ne 1){throw 'Open exactly one Teardown instance through Steam first.'}
$game=$games[0]
$profile=Resolve-VRBuild -ImageSize $game.MainModule.ModuleMemorySize
Write-Host "Selected $($profile.Name)"
Write-Host "Session log: $($profile.Log)"
if($Mode -eq 'Disable'){& $profile.Attach $game.Id --disable}else{& $profile.Attach $game.Id}
exit $LASTEXITCODE
