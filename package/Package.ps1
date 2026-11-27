$ErrorActionPreference='Stop'
$script:PackageRoot=$PSScriptRoot
function Get-FullPath([string]$Path){[IO.Path]::GetFullPath($Path).TrimEnd('\')}
function Get-ChildPath([string]$Root,[string]$Relative){
 $rootPath=Get-FullPath $Root;$path=Get-FullPath (Join-Path $rootPath $Relative)
 if(-not $path.StartsWith($rootPath+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'File path escaped its installation folder.'}
 $at=$path
 while($at.Length -ge $rootPath.Length){
  $entry=Get-Item -LiteralPath $at -Force -ErrorAction SilentlyContinue
  if($entry -and ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw "Linked installation path refused: $at"}
  if($at -eq $rootPath){break};$at=Split-Path $at -Parent
 }
 return $path
}
function Get-TextHash([string]$Text){
 $sha=[Security.Cryptography.SHA256]::Create()
 try{return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($Text)))).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose()}
}
function Get-BinaryHash([string]$Path){
 $sha=[Security.Cryptography.SHA256]::Create();$stream=[IO.File]::OpenRead($Path)
 try{return ([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-','').ToLowerInvariant()}finally{$stream.Dispose();$sha.Dispose()}
}
function Get-NormalText([string]$Text){$Text.Replace("`r`r`n","`r`n").Replace("`r`n","`n").Replace("`r","`n")}
function Write-VerifiedText([string]$Path,[string]$Text){
 [IO.File]::WriteAllText($Path,$Text,(New-Object Text.UTF8Encoding($false)))
 if([IO.File]::ReadAllText($Path) -cne $Text){throw "Write verification failed: $Path"}
}
function Get-Manifest {
 Get-Content -LiteralPath (Join-Path $script:PackageRoot 'manifest.json') -Raw | ConvertFrom-Json
}
function Confirm-Package {
 $manifest=Get-Manifest
 foreach($file in $manifest.files){
  $path=Get-ChildPath $script:PackageRoot $file.path
  if(-not(Test-Path -LiteralPath $path -PathType Leaf) -or (Get-BinaryHash $path) -ne $file.sha256){throw "Package file missing or damaged: $($file.path). Extract the ZIP again."}
 }
 return $manifest
}
function Find-Game([string]$Directory){
 if($Directory){$candidates=@($Directory)}else{
  $steamRoots=@()
  foreach($key in @('HKCU:\Software\Valve\Steam','HKLM:\SOFTWARE\WOW6432Node\Valve\Steam')){
   $entry=Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue
   if($entry){if($entry.SteamPath){$steamRoots+=$entry.SteamPath};if($entry.InstallPath){$steamRoots+=$entry.InstallPath}}
  }
  if(${env:ProgramFiles(x86)}){$steamRoots+=Join-Path ${env:ProgramFiles(x86)} 'Steam'}
  $libraries=@($steamRoots)
  foreach($steam in $steamRoots){
   $vdf=Join-Path $steam 'steamapps\libraryfolders.vdf'
   if(Test-Path -LiteralPath $vdf){foreach($match in [regex]::Matches([IO.File]::ReadAllText($vdf),'"path"\s+"([^"]+)"')){$libraries+=$match.Groups[1].Value.Replace('\\','\')}}
  }
  $candidates=@($libraries | Select-Object -Unique | ForEach-Object {Join-Path $_ 'steamapps\common\Teardown'})
 }
 foreach($candidate in $candidates){if(Test-Path -LiteralPath (Join-Path $candidate 'teardown.exe') -PathType Leaf){return Get-FullPath $candidate}}
 throw 'Teardown was not found in the Steam libraries. Run Install-VR.ps1 -GameDirectory with your Teardown folder.'
}
function Get-GameImageSize([string]$Game){
 $stream=[IO.File]::OpenRead((Get-ChildPath $Game 'teardown.exe'));$reader=New-Object IO.BinaryReader($stream)
 try{
  if($reader.ReadUInt16() -ne 0x5A4D){throw 'Invalid game executable.'}
  $stream.Position=0x3C;$pe=$reader.ReadInt32()
  if($pe -lt 64 -or $pe+84 -gt $stream.Length){throw 'Invalid executable header.'}
  $stream.Position=$pe;if($reader.ReadUInt32() -ne 0x4550){throw 'Invalid executable signature.'}
  $stream.Position=$pe+80;return [long]$reader.ReadUInt32()
 }finally{$reader.Dispose();$stream.Dispose()}
}
function Get-ScriptPlan([string]$Game,[switch]$Reverse){
 $specs=Get-Content -LiteralPath (Join-Path $script:PackageRoot 'patches\game-scripts.json') -Raw | ConvertFrom-Json
 foreach($spec in $specs){
  if($spec.name -notin @('player_locomotion','spawn')){throw 'Unknown game patch.'}
  $target=Get-ChildPath $Game ('data\script\'+$spec.name+'.lua')
  $raw=[IO.File]::ReadAllText($target);$text=Get-NormalText $raw;$hash=Get-TextHash $text
  $from=$spec.original_sha256;$to=$spec.patched_sha256
  if($Reverse){$from=$spec.patched_sha256;$to=$spec.original_sha256}
  if($hash -eq $to){continue}
  if($hash -ne $from){throw "Unrecognized $($spec.name).lua version or edits. No changes made. This game build needs a matching patch."}
  foreach($hunk in $spec.hunks){
   $before=[string]$hunk.before;$after=[string]$hunk.after
   if($Reverse){$before=[string]$hunk.after;$after=[string]$hunk.before}
   $index=$text.IndexOf($before,[StringComparison]::Ordinal)
   if(-not $before.Length -or $index -lt 0 -or $index -ne $text.LastIndexOf($before,[StringComparison]::Ordinal)){throw "Ambiguous $($spec.name) patch. No changes made."}
   $text=$text.Substring(0,$index)+$after+$text.Substring($index+$before.Length)
  }
  if((Get-TextHash $text) -ne $to){throw 'Game patch verification failed. No changes made.'}
  $eol=if($raw.Contains("`r`n")){"`r`n"}else{"`n"}
  [pscustomobject]@{Path=$target;Name=$spec.name;Text=$text.Replace("`n",$eol);OriginalHash=$spec.original_sha256}
 }
}
function Get-ModListChange([string]$Path,[switch]$Remove){
 $old=[IO.File]::ReadAllText($Path);$doc=New-Object Xml.XmlDocument;$doc.PreserveWhitespace=$true;$doc.LoadXml($old)
 $mods=$doc.SelectSingleNode('/modlist/mods');if(-not $mods){throw 'Unexpected mod-list format. No changes made.'}
 foreach($id in @('local-sledge-vr-accessories','local-sledge-vr-hyve')){
  $nodes=@($mods.SelectNodes("mod[@id='$id']"))
  if($Remove){foreach($node in $nodes){[void]$mods.RemoveChild($node)}}
  elseif($nodes.Count -eq 0){$node=$doc.CreateElement('mod');$node.SetAttribute('id',$id);[void]$mods.AppendChild($node)}
 }
 if($doc.OuterXml -cne $old){[pscustomobject]@{Path=$Path;Text=$doc.OuterXml}}
}
function Confirm-ModOwner([string]$Mods,[string]$Id,[string]$Name){
 $target=Get-ChildPath $Mods $Id
 if(Test-Path -LiteralPath $target){
  $info=Get-ChildPath $target 'info.txt'
  if(-not(Test-Path -LiteralPath $info) -or [IO.File]::ReadAllText($info) -notmatch ('(?m)^name\s*=\s*'+[regex]::Escape($Name)+'\s*$')){throw "Destination belongs to another mod: $Id"}
 }
 return $target
}
