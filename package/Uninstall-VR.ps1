param(
 [string]$GameDirectory,
 [string]$ModsDirectory=(Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'Teardown\mods'),
 [string]$ModListsDirectory=(Join-Path $env:LOCALAPPDATA 'Teardown\modlists'),
 [string]$RestoreDirectory=(Join-Path $env:LOCALAPPDATA 'TeardownVR\restore'),
 [switch]$Preview
)
. (Join-Path $PSScriptRoot 'Package.ps1')
if(Get-Process teardown -ErrorAction SilentlyContinue){throw 'Close Teardown before uninstalling.'}
$manifest=Get-Manifest;$game=Find-Game $GameDirectory
$patches=@(Get-ScriptPlan $game -Reverse)
$modRoots=@((Confirm-ModOwner $ModsDirectory 'sledge-vr-accessories' 'Sledge VR accessories'),(Confirm-ModOwner $ModsDirectory 'sledge-vr-hyve' 'Sledge VR hYve gun pack'))
$lists=@()
if(Test-Path -LiteralPath $ModListsDirectory){foreach($file in Get-ChildItem -LiteralPath $ModListsDirectory -Filter '*.xml' -File){$change=Get-ModListChange $file.FullName -Remove;if($change){$lists+=$change}}}
$remove=@();$directories=@($modRoots)
foreach($file in $manifest.files){
 if($file.path.StartsWith('mods/')){$target=Get-ChildPath $ModsDirectory $file.path.Substring(5);$directories+=Split-Path $target -Parent}
 elseif($file.path.StartsWith('runtime/')){$target=Get-ChildPath $PSScriptRoot $file.path}
 else{continue}
 if(Test-Path -LiteralPath $target -PathType Leaf){
  if((Get-BinaryHash $target) -eq $file.sha256){$remove+=$target}else{throw "Modified VR file preserved: $target. No changes made."}
 }
}
Write-Host "Restore $($patches.Count) script patches, remove $($remove.Count) VR files and update $($lists.Count) mod lists."
if($Preview){Write-Host 'Preview only. Nothing changed.';return}
foreach($patch in $patches){
 $backup=Get-ChildPath $RestoreDirectory ($patch.Name+'-'+$patch.OriginalHash+'.lua')
 if((Test-Path -LiteralPath $backup) -and (Get-TextHash (Get-NormalText ([IO.File]::ReadAllText($backup)))) -eq $patch.OriginalHash){Copy-Item -LiteralPath $backup -Destination $patch.Path -Force}
 else{Write-VerifiedText $patch.Path $patch.Text}
 if((Get-TextHash (Get-NormalText ([IO.File]::ReadAllText($patch.Path)))) -ne $patch.OriginalHash){throw 'Restored script verification failed.'}
}
foreach($list in $lists){Write-VerifiedText $list.Path $list.Text}
foreach($path in $remove){Remove-Item -LiteralPath $path -Force}
# Only empty, known mod directories are removed. No recursive deletion.
foreach($directory in ($directories | Select-Object -Unique | Sort-Object Length -Descending)){
 if((Test-Path -LiteralPath $directory) -and @(Get-ChildItem -LiteralPath $directory -Force).Count -eq 0){Remove-Item -LiteralPath $directory -Force}
}
Write-Host 'Teardown VR uninstalled. Saves, other mods and graphics settings were preserved.'
Write-Host 'To reinstall, extract the original ZIP again. Local restore copies remain outside this package.'
