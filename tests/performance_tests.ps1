$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$script=Join-Path $root 'package\Performance.ps1'
$testRoot=Join-Path $env:TEMP ('sledge-performance-test-'+[guid]::NewGuid())
New-Item -ItemType Directory -Path $testRoot | Out-Null
$fixture='<registry version="2.1.0"><options><gfx><renderscale value="100"/><quality value="3"/><particledensity value="1"/><dof value="1"/><gfxapi value="0"/><vsync value="1"/></gfx><input><sensitivity value="74"/></input><performance><renderscale value="0"/><quality value="99"/></performance></options></registry>'
$expected=@{Potato=@(50,1,0,0);Low=@(75,1,0,0);Medium=@(75,2,1,0);High=@(100,3,1,0);Ultra=@(100,3,1,1);Extreme=@(100,3,1,0)}
$count=0
foreach($preset in @('Potato','Low','Medium','High','Ultra','Extreme')){
 $path=Join-Path $testRoot ($preset+'.xml');$backups=Join-Path $testRoot ($preset+'-backups');[IO.File]::WriteAllText($path,$fixture)
 & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $script -Preset $preset -OptionsPath $path -BackupDirectory $backups | Out-Null
 if($LASTEXITCODE -ne 0){throw "Preset failed: $preset"}
 [xml]$actual=[IO.File]::ReadAllText($path);$g=$actual.registry.options.gfx;$e=$expected[$preset]
 if($g.renderscale.value -ne $e[0] -or $g.quality.value -ne $e[1] -or $g.particledensity.value -ne $e[2] -or $g.dof.value -ne $e[3] -or $g.vsync.value -ne '0' -or $g.gfxapi.value -ne '0' -or $actual.registry.options.input.sensitivity.value -ne '74' -or $actual.registry.options.performance.renderscale.value -ne '0' -or $actual.registry.options.performance.quality.value -ne '99'){throw "Incorrect settings: $preset"}
 $count++;Write-Host "PASS $preset values and unrelated setting preservation"
 & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $script -Preset Restore -OptionsPath $path -BackupDirectory $backups | Out-Null
 if($LASTEXITCODE -ne 0 -or [IO.File]::ReadAllText($path) -cne $fixture){throw "Restore failed: $preset"}
 $count++;Write-Host "PASS $preset exact backup restore"
}
Write-Host "$count checks passed. Fixtures: $testRoot"

