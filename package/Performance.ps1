param(
 [ValidateSet('Potato','Low','Medium','High','Ultra','Extreme','Restore','Preview')][string]$Preset,
 [string]$OptionsPath = (Join-Path $env:LOCALAPPDATA 'Teardown\options.xml'),
 [string]$BackupDirectory = (Join-Path $env:LOCALAPPDATA 'TeardownVR\graphics-restore')
)
$ErrorActionPreference='Stop'
# Values come from the installed game's graphic_options_logic.lua, not guessed keys.
$presets=[ordered]@{
 Potato=@{renderscale=50;quality=1;particledensity=0;dof=0}
 Low=@{renderscale=75;quality=1;particledensity=0;dof=0}
 Medium=@{renderscale=75;quality=2;particledensity=1;dof=0}
 High=@{renderscale=100;quality=3;particledensity=1;dof=0}
 Ultra=@{renderscale=100;quality=3;particledensity=1;dof=1}
 Extreme=@{renderscale=100;quality=3;particledensity=1;dof=0}
}
Write-Host 'VR performance presets (game resolution scale, quality, particles, depth of field)'
foreach($name in $presets.Keys){$p=$presets[$name];Write-Host ('{0,-7} {1,3}%  quality {2}/3  particles {3}/1  depth-of-field {4}' -f $name,$p.renderscale,$p.quality,$p.particledensity,$p.dof)}
Write-Host 'All presets use OpenGL and disable desktop VSync. High and Ultra use the engine highest quality.'
Write-Host 'Ultra retains depth of field. Presets cannot guarantee a frame rate or remove heavy mod workloads.'
Write-Host 'Extreme uses native 100% game render scale and maximum quality without depth-of-field blur. Headset 1:1 pixels still depend on game resolution and SteamVR; this preset does not increase the render target size.'
if($Preset -eq 'Preview'){exit 0}
if(-not $Preset){
 Write-Host '1 Potato   2 Low   3 Medium   4 High   5 Ultra   6 Extreme   7 Restore backup   0 Cancel'
 $choice=Read-Host 'Choose a number'
 $choices=@{'1'='Potato';'2'='Low';'3'='Medium';'4'='High';'5'='Ultra';'6'='Extreme';'7'='Restore'}
 if(-not $choices.ContainsKey($choice)){exit 0};$Preset=$choices[$choice]
}
if(Get-Process teardown -ErrorAction SilentlyContinue){throw 'Close Teardown before changing graphics settings.'}
$path=[IO.Path]::GetFullPath($OptionsPath)
if(-not (Test-Path -LiteralPath $path -PathType Leaf)){throw 'Teardown settings were not found. Launch and close the game once first.'}
$old=[IO.File]::ReadAllText($path)
[xml]$document=$old
if(-not $document.registry.options.gfx){throw 'Unexpected settings file; nothing changed.'}
New-Item -ItemType Directory -Path $BackupDirectory -Force | Out-Null
if($Preset -eq 'Restore'){
 $restore=Get-ChildItem -LiteralPath $BackupDirectory -Filter 'options-*.xml' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
 if(-not $restore){throw 'No settings backup exists.'}
 $next=[IO.File]::ReadAllText($restore.FullName);[xml]$validated=$next
 if(-not $validated.registry.options.gfx){throw 'Invalid backup.'}
 [IO.File]::WriteAllText((Join-Path $BackupDirectory ('before-restore-'+[guid]::NewGuid().ToString()+'.xml')),$old)
}else{
 $values=@{};foreach($key in $presets[$Preset].Keys){$values[$key]=$presets[$Preset][$key]};$values.gfxapi=0;$values.vsync=0
 # Other sections (including mod settings) can have the same tag names.
 # Match only the unique options/gfx block, preserving the rest byte-for-byte.
 $gfxMatches=[regex]::Matches($old,'(?s)<gfx(?:\s[^>]*)?>.*?</gfx>')
 if($gfxMatches.Count -ne 1){throw 'Expected one graphics settings block. Nothing changed.'}
 $gfx=$gfxMatches[0];$nextGfx=$gfx.Value
 foreach($key in $values.Keys){
  $pattern='(<'+$key+'\s+value=")[0-9]+("\s*/>)'
  if(([regex]::Matches($nextGfx,$pattern)).Count -ne 1){throw "Unexpected or missing graphics setting: $key. Nothing changed."}
  $nextGfx=[regex]::Replace($nextGfx,$pattern,('${1}'+$values[$key]+'${2}'))
 }
 $next=$old.Substring(0,$gfx.Index)+$nextGfx+$old.Substring($gfx.Index+$gfx.Length)
 $backup=Join-Path $BackupDirectory ('options-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'.xml')
 [IO.File]::WriteAllText($backup,$old,(New-Object Text.UTF8Encoding($false)))
}
[IO.File]::WriteAllText($path,$next,(New-Object Text.UTF8Encoding($false)))
if([IO.File]::ReadAllText($path) -cne $next){throw 'Settings verification failed.'}
Write-Host "$Preset applied and verified. Launch Teardown before starting SteamVR."
