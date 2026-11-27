function Resolve-VRBuild {
 param([long]$ImageSize,[string]$PackageDirectory=$PSScriptRoot)
 # Image size only selects a candidate. Its native launcher verifies every
 # expected game signature before it loads or changes anything in the process.
 switch($ImageSize){
  0x2092000 {$relative='runtime\current';$label='2.2 / Steam build 26758720 (r7.34)'}
  0x20A4000 {$relative='runtime\teardown-26320217';$label='2.2 / Steam build 26320217 (r7.23)'}
  0x204E000 {$relative='runtime\teardown-21';$label='retained 2.1 build (r7.20)'}
  default {throw ('Unsupported Teardown build (image size 0x{0:X}). No VR files loaded.' -f $ImageSize)}
 }
 $bin=Join-Path $PackageDirectory $relative
 foreach($name in @('SledgePortAttach.exe','SledgePort21.dll','openvr_api.dll')){
  if(-not(Test-Path -LiteralPath (Join-Path $bin $name) -PathType Leaf)){throw "Missing VR runtime file: $bin\$name"}
 }
 [pscustomobject]@{Name=$label;Bin=$bin;Attach=(Join-Path $bin 'SledgePortAttach.exe');Log=(Join-Path $bin 'SledgePort21.log')}
}
