param([Parameter(Mandatory=$true)][string]$LogPath)
$ErrorActionPreference = 'Stop'
$lines = Get-Content -LiteralPath $LogPath
$gaps = @($lines | ForEach-Object { if ($_ -match '^(\d+).*frame-gap=([\d.]+)') { [pscustomobject]@{Time=[long]$Matches[1];Ms=[double]::Parse($Matches[2],[cultureinfo]::InvariantCulture)} } })
$slow = @($lines | Where-Object { $_ -match 'Slow |Stall watchdog:' })
if (!$gaps.Count) {
 Write-Output ($lines | Select-Object -First 1)
 Write-Output 'No frame timing samples; headset performance is unverified.'
 if($slow.Count){$slow | Write-Output;Write-Output 'REVIEW REQUIRED: watchdog warnings exist without headset timing samples.';exit 1}
 exit 2
}
$maximum = ($gaps | Measure-Object Ms -Maximum).Maximum
$stalls = @($gaps | Where-Object Ms -ge 250)
Write-Output ($lines | Select-Object -First 1)
Write-Output "Timing windows: $($gaps.Count); max frame gap: $maximum ms; windows >=250 ms: $($stalls.Count)"
$stalls | Format-Table -AutoSize
$slow | Write-Output
Write-Output 'Frame gaps include loading, focus and tracking changes. This log cannot identify the cause by itself.'
if ($stalls.Count -or $slow.Count) { Write-Output 'REVIEW REQUIRED: do not mark freezing resolved.'; exit 1 }
Write-Output 'No recorded >=250 ms stalls. This does not guarantee future sessions are freeze-free.'
