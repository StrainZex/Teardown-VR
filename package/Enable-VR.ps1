$ErrorActionPreference = 'Stop'
# Start Teardown before SteamVR on this Virtual Desktop setup.
$games = @(Get-Process teardown -ErrorAction SilentlyContinue)
$runtimeFile=Join-Path $env:LOCALAPPDATA 'openvr\openvrpaths.vrpath'
$runtime=$null
if(Test-Path -LiteralPath $runtimeFile){
 foreach($candidate in (Get-Content -LiteralPath $runtimeFile -Raw | ConvertFrom-Json).runtime){
  if(Test-Path -LiteralPath (Join-Path $candidate 'bin\win64\vrstartup.exe')){$runtime=$candidate;break}
 }
}
if($games.Count -eq 0){
 if(Get-Process vrserver -ErrorAction SilentlyContinue){
  Write-Host 'Closing SteamVR so Teardown can start normally...'
  Get-Process vrmonitor -ErrorAction SilentlyContinue | ForEach-Object { [void]$_.CloseMainWindow() }
  $deadline=(Get-Date).AddSeconds(30)
  while((Get-Process vrserver -ErrorAction SilentlyContinue) -and (Get-Date) -lt $deadline){Start-Sleep -Milliseconds 500}
  if(Get-Process vrserver -ErrorAction SilentlyContinue){throw 'SteamVR did not close normally. Exit SteamVR, then click Enable-VR again.'}
 }
 Write-Host 'Starting Teardown. Choose a level in the game; this launcher will handle SteamVR and attachment.'
 Start-Process 'steam://rungameid/1167630' -WindowStyle Hidden
 $deadline=(Get-Date).AddMinutes(3)
 do{Start-Sleep -Milliseconds 500;$games=@(Get-Process teardown -ErrorAction SilentlyContinue)}while($games.Count -eq 0 -and (Get-Date) -lt $deadline)
}
if($games.Count -ne 1){throw 'Could not find exactly one Teardown process.'}
Start-Sleep -Seconds 3
$game = $games[0]
. (Join-Path $PSScriptRoot 'Resolve-VRBuild.ps1')
$profile=Resolve-VRBuild -ImageSize $game.MainModule.ModuleMemorySize
Write-Host "Selected $($profile.Name)"
$attach = $profile.Attach
$log = $profile.Log
& $attach $game.Id
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$verified = $false
Write-Host "Waiting for a loaded OpenGL level..."
for ($attempt = 0; $attempt -lt 900; $attempt++) {
    Start-Sleep -Seconds 1
    if($game.HasExited){throw "Teardown closed before VR could start."}
    if (Test-Path -LiteralPath $log) {
        $file = Get-Item -LiteralPath $log
        $lines = Get-Content -LiteralPath $log -Raw
        if ($file.LastWriteTime -ge $game.StartTime -and $lines -match 'cameraValid=1 OpenGLContext=1') {
            $verified = $true
            break
        }
    }
}
if (-not $verified) {
    Write-Host 'VR was not enabled: no valid OpenGL scene was detected.'
    Write-Host 'Keep a level loaded and use the Steam launch option: -gfxapi OpenGL'
    if (Test-Path -LiteralPath $log) { Get-Content -LiteralPath $log -Tail 8 }
    exit 3
}
if(-not (Get-Process vrserver -ErrorAction SilentlyContinue)){
 Write-Host 'Starting SteamVR...'
 if($runtime){Start-Process -FilePath (Join-Path $runtime 'bin\win64\vrstartup.exe') -WindowStyle Hidden}
 else{Start-Process 'steam://rungameid/250820' -WindowStyle Hidden}
 $deadline=(Get-Date).AddSeconds(90)
 while(-not (Get-Process vrcompositor -ErrorAction SilentlyContinue)){
  if((Get-Date) -gt $deadline){throw 'SteamVR did not become ready. Check the Virtual Desktop headset connection.'}
  Start-Sleep -Milliseconds 500
 }
 Start-Sleep -Seconds 5
}
& $attach $game.Id --stereo
$result=$LASTEXITCODE
if($result -eq 0){try{$focus=New-Object -ComObject WScript.Shell;[void]$focus.AppActivate($game.Id)}catch{Write-Host 'Select the Teardown window for controller input.'}}
exit $result
