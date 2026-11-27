param(
 [string]$GameDirectory,
 [string]$ModsDirectory=(Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'Teardown\mods'),
 [string]$ModListPath=(Join-Path $env:LOCALAPPDATA 'Teardown\modlists\1.xml'),
 [string]$RestoreDirectory=(Join-Path $env:LOCALAPPDATA 'TeardownVR\restore'),
 [switch]$Preview
)
. (Join-Path $PSScriptRoot 'Package.ps1')
if(Get-Process teardown -ErrorAction SilentlyContinue){throw 'Close Teardown before installing.'}
$manifest=Confirm-Package;$game=Find-Game $GameDirectory
. (Join-Path $PSScriptRoot 'Resolve-VRBuild.ps1')
$build=Resolve-VRBuild -ImageSize (Get-GameImageSize $game)
$patches=@(Get-ScriptPlan $game)
if(-not(Test-Path -LiteralPath $ModListPath)){throw 'Launch and close Teardown once to create its Default mod list, then run Install-VR again.'}
$list=Get-ModListChange $ModListPath
[void](Confirm-ModOwner $ModsDirectory 'sledge-vr-accessories' 'Sledge VR accessories')
[void](Confirm-ModOwner $ModsDirectory 'sledge-vr-hyve' 'Sledge VR hYve gun pack')
$copies=@()
foreach($file in $manifest.files){
 if($file.path.StartsWith('mods/')){
  $relative=$file.path.Substring(5)
  $copies+=[pscustomobject]@{Source=(Get-ChildPath $PSScriptRoot $file.path);Target=(Get-ChildPath $ModsDirectory $relative);Hash=$file.sha256}
 }
}
Write-Host "Install $($copies.Count) mod files and $($patches.Count) game-script patches for $($build.Name)."
if($Preview){Write-Host 'Preview only. Nothing changed.';return}
# Recipient-only restore copies are created here; none are distributed in the ZIP.
foreach($patch in $patches){
 $backup=Get-ChildPath $RestoreDirectory ($patch.Name+'-'+$patch.OriginalHash+'.lua')
 if(-not(Test-Path -LiteralPath $backup)){New-Item -ItemType Directory -Path (Split-Path $backup -Parent) -Force | Out-Null;Copy-Item -LiteralPath $patch.Path -Destination $backup}
}
foreach($copy in $copies){
 New-Item -ItemType Directory -Path (Split-Path $copy.Target -Parent) -Force | Out-Null
 Copy-Item -LiteralPath $copy.Source -Destination $copy.Target -Force
 if((Get-BinaryHash $copy.Target) -ne $copy.Hash){throw 'Installed mod verification failed.'}
}
foreach($patch in $patches){Write-VerifiedText $patch.Path $patch.Text}
if($list){Write-VerifiedText $list.Path $list.Text}
Write-Host 'Teardown VR installed. In Steam launch options, use: -gfxapi OpenGL'
Write-Host 'Enable the two Sledge VR mods in your active mod list, then use Enable-VR.cmd.'
Write-Host 'The Default mod list has been updated. Saves and other mods were not replaced.'
