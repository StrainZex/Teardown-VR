from pathlib import Path
import sys,xml.etree.ElementTree as E
from lupa.lua51 import LuaRuntime
r=Path('mods/sledge-vr-hyve')
for p in r.glob('*.xml'):
 t=E.parse(p);assert not any(x.tag in ('script','joint','rope') for x in t.iter());assert len(list(t.iter('body')))==1
 for v in t.iter('vox'):assert '/gore/' not in v.get('file','');assert (r/v.get('file').removeprefix('MOD/')).exists()
print('PASS five independent weapon prefabs with no character bodies, scripts, joints or ropes')
lua=LuaRuntime(unpack_returned_tuples=True)
# Reuse only the tested vector/quaternion mock helpers, not companion behavior.
base=Path('tests/companion_tests.py').read_text().split("lua.execute(r'''")[1].split('sample={')[0]
lua.execute(base)
lua.execute(r'''
clock=0;on=0;restricted=0;selected='sledge';registry={['game.tool.sledge.enabled']=true,['game.tool.custom.enabled']=true,['game.tool.locked.enabled']=false};registered={};shots={};sounds=0;fires=0;pulses=0;rayhit=true;body=42
sample={717,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,1,0,0}
tool=Transform(Vec(5,2,3),QuatEuler(0,90,0));muzzle=Transform(Vec(5,2,2.5),tool.rot)
function GetPlayerHealth(_,op) if op==12 then return 723,on,restricted elseif op==1 then return unpack(sample) elseif op==3 then pulses=pulses+1 end end
registryWrites=0
function SetBool(k,v) registry[k]=v;registryWrites=registryWrites+1 end
function GetBool(k) return registry[k] or false end
function RegisterTool(id,name,path,group) registered[id]=path end
function LoadSound(s)return s end
function LoadSprite(s)return s end
sprites={};lights={};spawnCount=0;spawnAttempts=0;failSpawn=false;propertyWrites=0
function Spawn(xml,t,static)
 spawnAttempts=spawnAttempts+1
 assert(xml:find('type="cone"') and not xml:find('<body') and not xml:find('<vox'))
 assert(not xml:find('scale="0"'));assert(static)
 if failSpawn then return {} end
 spawnCount=spawnCount+1;local h=100+spawnCount
 lights[h]={xml=xml,pose=t,enabled=true};return {h}
end
function GetEntityType(h)return lights[h] and 'light' or 'body' end
function SetProperty(h,key,t)assert(lights[h] and key=='transform');lights[h].pose=t;propertyWrites=propertyWrites+1 end
function SetLightEnabled(h,on)assert(lights[h]);lights[h].enabled=on end
function Delete(h)lights[h]=nil end
function DrawSprite(_,t,w,h,r,g,b,a,depth,add) sprites[#sprites+1]={texture=_,t=t,w=w,h=h,r=r,g=g,b=b,a=a,depth=depth,add=add} end
function PointLight()end
function GetPlayerTool()return selected end
function SetPlayerTool(s)selected=s end
function ListKeys()return {'sledge','custom','locked','vrhands','hyve_glock','hyve_g36','hyve_magnum','hyve_shotgun','hyve_flame'} end
function GetTime()return clock end
function IsHandleValid(h)return h==42 or h==43 or h==44 or lights[h]~=nil end
shapeQueries=0;emission={}
function GetBodyShapes(h) assert(h==42);shapeQueries=shapeQueries+1;return {43,44} end
function HasTag(h,t) return h==43 and t=='vr_sight_glow' end
function SetShapeEmissiveScale(h,s) assert(h==43);emission[h]=s end
function GetToolBody()return body end
function GetBodyTransform()return tool end
function GetToolLocationWorldTransform()return muzzle end
function SetToolTransform()end
function SetToolHandPoseLocalTransform(p) handPose=p end
function QueryRejectBody()end
function GetLocalPlayer()return 1 end
function QueryRejectPlayer()end
function QueryRaycast(p,d,range) shots[#shots+1]={p=p,d=d,range=range,kind='ray'};return rayhit,rayDistance or 3,Vec(0,1,0) end
function Shoot(p,d,kind,strength,range,player)
 error('hYve bullets must not be replaced with default projectiles')
end
holes=0
impacts={};particles={}
function MakeHole(p,soft,medium)holes=holes+1;impacts[#impacts+1]={p=p,soft=soft,medium=medium} end
function SpawnFire()fires=fires+1 end
switches={}
function PlaySound(handle,pos,volume,register)
 sounds=sounds+1
 if handle=='clickdown.ogg' or handle=='clickup.ogg' then switches[#switches+1]={handle=handle,pos=pos,volume=volume,register=register} end
end
function SpawnParticle(p,v,life)particles[#particles+1]={p=p,v=v,life=life,radius=particleRadius,color=particleColor,emission=particleEmission} end
function ParticleReset()end
function ParticleType()end
function ParticleColor(r,g,b)particleColor={r,g,b} end
function ParticleRadius(r)particleRadius=r end
function ParticleAlpha()end
function ParticleCollide()end
function ParticleEmissive(v)particleEmission=v end
''')
lua.execute((r/'main.lua').read_text())
lua.execute(r'''
sample[34]=0;sample[35]=0;init();tick(.016);assert(not registry['game.tool.hyve_glock.enabled'])
on=1;tick(.016);assert(registry['game.tool.hyve_glock.enabled'] and registry['game.tool.custom.enabled'] and registry['game.tool.sledge.enabled'])
restricted=1;tick(.016);assert(selected=='vrhands' and not registry['game.tool.custom.enabled'])
restricted=0;tick(.016);assert(registry['game.tool.custom.enabled'] and registry['game.tool.sledge.enabled'] and not registry['game.tool.locked.enabled'])
selected='hyve_glock';sample[28]=1;tick(.016);assert(#shots==1 and holes==1 and shots[1].kind=='ray' and math.abs(shots[1].d[1]+1)<.0001 and math.abs(shots[1].d[3])<.0001);assert(shots[1].p[1]==5 and shots[1].p[3]==2.5)
assert(impacts[1].soft==.1 and impacts[1].medium==.1 and math.abs(impacts[1].p[2]-1.995)<.0001)
assert(#particles==1 and particles[1].radius==.025 and particles[1].life==.15 and math.abs(VecLength(particles[1].v)-80)<.0001)
assert(particles[1].color[1]==1 and particles[1].color[2]==.2 and particles[1].color[3]==0 and particles[1].emission==5)
tick(.016);assert(#shots==1);clock=10;tick(.016);assert(#shots==2)
clock=11;sample[30]=1;tick(.016);assert(#shots==2);sample[30]=0
sample[24]=0;tick(.016);assert(#shots==2);sample[24]=1
sample[29]=1;tick(.016);assert(#shots==2);sample[29]=0
sample[32]=2;tick(.016);assert(#shots==2);sample[32]=0
selected='hyve_shotgun';clock=12;local before=#shots;tick(.016);assert(#shots-before==8);sample[28]=0;for i=1,8 do clock=clock+.016;local n=#shots;tick(.016);assert(#shots-n<=8) end;assert(#shots-before==65)
selected='hyve_shotgun';clock=14;sample[28]=1;tick(.016);local n=#shots;selected='sledge';tick(.016);assert(#shots==n)
selected='hyve_magnum';clock=15;tick(.016);assert(#shots-n==6)
selected='hyve_flame';clock=16;tick(.016);assert(fires==1 and shots[#shots].range==4)
clock=17;rayhit=false;tick(.016);assert(fires==1)
restricted=1;tick(.016);on=0;tick(.016);assert(selected=='vrhands' and registry['game.tool.custom.enabled'] and not registry['game.tool.hyve_glock.enabled'])
''')
print('PASS pack toggle preserves other mods; restricted mode restores enabled/locked tools; controller direction and muzzle; cadence; no catch-up; menus/tracking/vehicle/grab guards; 65 bounded pellets; switch cancellation; magnum; flame range/misses; disable cleanup')

lua.execute(r'''
on=1;restricted=0;selected='hyve_g36';sample[28]=0;sample[27]=0;clock=30;rayhit=true;tick(.016)
assert(math.abs(handPose.pos[2]-.57)<.00001 and spawnCount==0)
local count=#sprites;local rays=#shots
sample[27]=1;sample[25]=1;sample[35]=0;tick(.016);assert(#sprites==count and #shots==rays)
sample[35]=1;tick(.016);assert(#sprites==count and #switches==0) -- press before support must not toggle
sample[27]=0;tick(.016);sample[27]=1;tick(.016)
assert(spawnCount==0 and #sprites==count+1 and #shots==rays+1);assert(#switches==1 and switches[1].handle=="clickdown.ogg")
local dot=sprites[#sprites];assert(dot.texture=='gfx/glare.png' and dot.w==.12 and dot.h==.12 and dot.r==4 and dot.g==0 and dot.b==0 and dot.depth and dot.add)
local direction=TransformToParentVec(tool,Vec(0,0,-1))
assert(VecDot(direction,shots[#shots].d)>.999 and shots[#shots].range==80)
local expected=VecAdd(VecAdd(shots[#shots].p,VecScale(direction,3)),Vec(0,.004,0))
assert(VecLength(VecSub(dot.t.pos,expected))<.00001)
local n=pulses;count=#sprites;rays=#shots
for i=1,20 do tick(.016)end
assert(pulses==n and #sprites==count+20 and #shots==rays+20 and spawnCount==0 and #switches==1)
count=#sprites;rayhit=false;tick(.016);assert(#sprites==count);rayhit=true;rayDistance=81;tick(.016);assert(#sprites==count);rayDistance=nil
sample[30]=1;rays=#shots;tick(.016);assert(#sprites==count and #shots==rays);sample[30]=0
sample[24]=0;tick(.016);assert(#sprites==count);sample[24]=1
sample[27]=0;tick(.016);sample[27]=1;count=#sprites;rays=#shots;tick(.016);assert(#sprites==count and #shots==rays)
sample[27]=0;tick(.016);sample[27]=1;tick(.016);assert(#sprites==count+1)
sample[27]=0;sample[28]=1;clock=40;count=#sprites;tick(.016);assert(#sprites==count+4) -- dot plus three muzzle quads
sample[28]=0;clock=40.1;count=#sprites;tick(.016);assert(#sprites==count+1) -- flash expired
selected='hyve_magnum';sample[27]=0;count=#sprites;tick(.016);assert(math.abs(handPose.pos[3]+.41)<.00001 and math.abs(handPose.pos[2]-.03)<.00001 and #sprites==count)
selected='hyve_glock';sample[27]=0;tick(.016);assert(math.abs(handPose.pos[3]-.265)<.00001 and spawnCount==0)
sample[27]=1;tick(.016);assert(spawnCount==1 and lights[101].enabled)
assert(lights[101].xml:find('color="0.81 0.9 1"') and lights[101].xml:find('angle="15"'))
assert(VecDot(VecSub(lights[101].pose.pos,muzzle.pos),direction)>.149)
for i=1,20 do tick(.016)end;assert(spawnCount==1 and #switches==4)
assert(switches[2].handle=='clickup.ogg' and switches[3].handle=='clickdown.ogg' and switches[4].handle=='clickdown.ogg')
sample[27]=0;tick(.016);sample[27]=1;tick(.016);assert(not lights[101].enabled and #switches==5 and switches[5].handle=='clickup.ogg')
sample[27]=0;tick(.016);sample[27]=1;tick(.016);assert(lights[101].enabled and #switches==6 and switches[6].handle=='clickdown.ogg')
selected='sledge';tick(.016);assert(not lights[101].enabled)
lights[101]=nil;selected='hyve_glock';clock=50;failSpawn=true;tick(.016);local attempts=spawnAttempts
for i=1,20 do tick(.016)end;assert(spawnAttempts==attempts)
clock=52.1;failSpawn=false;tick(.016);assert(spawnCount==2 and lights[102].enabled)
on=0;tick(.016);assert(not lights[102].enabled);shutdown();assert(next(lights)==nil and #switches==6)
for _,click in ipairs(switches) do assert(VecLength(VecSub(click.pos,tool.pos))<.0001 and click.volume==.7 and click.register==false) end
''')
print('PASS revised grips; surface-only laser dot at first hit; no G36 cone/beam; miss/range/menu/tracking/toggle guards; bounded ray count; Glock flashlight and cleanup; one positional on/off click per accepted toggle with no repeats or automatic-cleanup clicks')

lua.execute(r'''
on=1;restricted=0;selected='hyve_glock';sample[28]=0;sample[27]=0;clock=100;tick(.016)
local queries=shapeQueries
for i=1,50 do tick(.016) end
assert(shapeQueries==queries and emission[43]==.8 and emission[44]==nil)
-- A short tap inside cooldown is retained once, even after release.
local n=#shots;local soundCount=sounds
sample[28]=1;tick(.016);assert(#shots==n+1 and sounds==soundCount+1)
clock=100.05;sample[28]=0;tick(.016)
clock=100.1;sample[28]=1;tick(.016);assert(#shots==n+1)
clock=100.12;sample[28]=0;tick(.016)
clock=100.21;tick(.016);assert(#shots==n+2 and sounds==soundCount+2)
clock=100.8;tick(.016);assert(#shots==n+2)
-- A released tap must not unexpectedly fire after a long freeze.
clock=101;sample[28]=1;tick(.016);sample[28]=0;clock=101.05;tick(.016)
sample[28]=1;clock=101.1;tick(.016);sample[28]=0
n=#shots;clock=104;tick(.016);assert(#shots==n)
-- Menu/tracking cancellation removes buffered input.
sample[28]=1;clock=105;tick(.016);sample[28]=0;clock=105.05;tick(.016)
sample[28]=1;clock=105.1;tick(.016);sample[30]=1;tick(.016)
sample[30]=0;sample[28]=0;clock=106;n=#shots;tick(.016);assert(#shots==n)
-- A slow frame cannot replace an unfinished shotgun volley with a new one.
selected='hyve_shotgun';clock=110;sample[28]=1;n=#shots;local holeStart=holes;local particleStart=#particles;tick(.016)
for i=1,8 do clock=clock+1;local previous=#shots;tick(1);assert(#shots-previous<=8) end
assert(#shots-n==65)
assert(holes-holeStart==65 and #particles-particleStart==65)
for i=n+1,#shots do assert(shots[i].kind=='ray' and shots[i].range==150) end
for i=holeStart+1,holes do assert(impacts[i].soft==.15 and impacts[i].medium==.15) end
clock=clock+1;tick(1);assert(#shots-n==73)
-- Switching tools discards the remainder, including during a frozen frame.
selected='sledge';sample[28]=0;n=#shots;tick(.016);clock=130;tick(.016);assert(#shots==n)
-- Restricted mode should not rewrite already-disabled registry entries.
restricted=1;tick(.016);local writes=registryWrites
clock=131;tick(.016);assert(registryWrites==writes)
registry['game.tool.custom.enabled']=true;clock=132;tick(.016)
assert(registryWrites==writes+1 and not registry['game.tool.custom.enabled'])
restricted=0;tick(.016);assert(registry['game.tool.custom.enabled'])
-- A miss still emits the original tracer, but cannot create a false impact.
selected='hyve_glock';clock=140;sample[28]=1;rayhit=false
n=#shots;holeStart=holes;particleStart=#particles;tick(.016)
assert(#shots==n+1 and holes==holeStart and #particles==particleStart+1)
shutdown()
''')
print('PASS original hYve tracer and damage sizes, surface hit placement, miss handling, buffered taps, stale/menu cancellation, all 65 impacts/tracers at low FPS, cached sight emission and bounded inventory writes')
