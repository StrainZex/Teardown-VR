from pathlib import Path
import sys
from lupa.lua51 import LuaRuntime
lua=LuaRuntime(unpack_returned_tuples=True)
lua.execute(r'''
now=0;enabled=true;pulses={0,0};selected="sledge";objects={};serial=10;report={};emergency=0;damage=0;attacks=0;registry={};modeCycle=0
function Vec(x,y,z)return {x or 0,y or 0,z or 0}end
function VecStr(v)return table.concat(v,",")end
function DebugPrint()end
function VecCopy(v)return Vec(v[1],v[2],v[3])end
function VecAdd(a,b)return Vec(a[1]+b[1],a[2]+b[2],a[3]+b[3])end
function VecSub(a,b)return Vec(a[1]-b[1],a[2]-b[2],a[3]-b[3])end
function VecScale(a,b)return Vec(a[1]*b,a[2]*b,a[3]*b)end
function VecLength(v)return math.sqrt(v[1]^2+v[2]^2+v[3]^2)end
function VecNormalize(v)return VecScale(v,1/VecLength(v))end
function VecDot(a,b)return a[1]*b[1]+a[2]*b[2]+a[3]*b[3]end
function Quat(x,y,z,w)return {x or 0,y or 0,z or 0,w or 1}end
function QMul(a,b)return {a[4]*b[1]+a[1]*b[4]+a[2]*b[3]-a[3]*b[2],a[4]*b[2]-a[1]*b[3]+a[2]*b[4]+a[3]*b[1],a[4]*b[3]+a[1]*b[2]-a[2]*b[1]+a[3]*b[4],a[4]*b[4]-a[1]*b[1]-a[2]*b[2]-a[3]*b[3]}end
function QInv(q)return {-q[1],-q[2],-q[3],q[4]}end
function QVec(q,v)local t=QMul(QMul(q,{v[1],v[2],v[3],0}),QInv(q));return Vec(t[1],t[2],t[3])end
function QuatRotateQuat(a,b)return QMul(a,b)end
function QuatEuler(x,y,z)return Quat(0,math.sin(y*math.pi/360),0,math.cos(y*math.pi/360))end
function HasTag(h,tag)return toolTag or false end
function GetString(key)return label or "" end
function GetInt(key)return registry[key] or 0 end
function SetInt(key,value)registry[key]=value end
function QuatLookAt(_,f)local angle=math.atan2(-f[1],-f[3]);return Quat(0,math.sin(angle/2),0,math.cos(angle/2))end
function Transform(p,r)return {pos=p or Vec(),rot=r or Quat()}end
function TransformToParentTransform(a,b)return Transform(VecAdd(a.pos,QVec(a.rot,b.pos)),QMul(a.rot,b.rot))end
function TransformToLocalTransform(a,b)return Transform(QVec(QInv(a.rot),VecSub(b.pos,a.pos)),QMul(QInv(a.rot),b.rot))end
function TransformToParentPoint(a,b)return VecAdd(a.pos,QVec(a.rot,b))end
function TransformToLocalPoint(a,b)return QVec(QInv(a.rot),VecSub(b,a.pos))end
function TransformToParentVec(a,b)return QVec(a.rot,b)end
sample={717,-1,1,0,0,0,0,1,1,1,0,0,0,0,1,0,1.7,0,0,0,0,1,1,1,0,0,0,0,0,0,1,0,1}
function GetPlayerHealth(marker,op,...)
 local a={...}
 if op==0 then return 717,enabled and 1 or 0 end
 if op==1 then return unpack(sample) end
 if op==2 then assert(#a==17,"pose payload must include all quaternion components");report=a;local e=emergency;emergency=0;return e end
 if op==3 then pulses[a[1]+1]=pulses[a[1]+1]+1;return 1 end
 if op==10 then attacks=attacks+1;return 1 end
 if op==11 then currentMode=a[1];local c=modeCycle;modeCycle=0;return c end
 if op==4 then report={0,0,-1};return 1 end
 return 0
end
function IsHandleValid(h)return objects[h]~=nil end
function GetEntityType(h)return objects[h].kind end
function Spawn(xml,t,allowStatic)
 assert(allowStatic==true,"accessories must explicitly allow static bodies")
 serial=serial+1;objects[serial]={kind="body",pose=t,shapes={},dynamic=false};local body=serial
 if xml:find('tablet') then serial=serial+1;objects[serial]={kind="shape",body=body};table.insert(objects[body].shapes,serial);tabletBody=body end
 return {body}
end
function SetTag()end
function RemoveTag()end
function GetBodyShapes(h)return objects[h].shapes end
function SetShapeCollisionFilter(h,layer,mask)objects[h].layer=layer;objects[h].mask=mask end
function GetBodyBounds(h)local p=objects[h].pose.pos;return VecAdd(p,Vec(-.02,-.09,-.135)),VecAdd(p,Vec(.02,.09,.135))end
function SetShapeLocalTransform(h,t)objects[h].pose=t end
function GetBodyTransform(h)return objects[h].pose end
function SetBodyTransform(h,t)objects[h].pose=t end
function SetBodyAngularVelocity(h,v)objects[h].angular=v end
function SetBodyDynamic(h,v)objects[h].dynamic=v end
function SetBodyVelocity(h,v)objects[h].velocity=v end
function GetBodyClosestPoint(h,p)return true,objects[h].pose.pos,Vec(),objects[h].shapes[1]end
function GetPlayerTransform()return Transform(Vec(sample[16],0,sample[18]),playerRotation or Quat())end
function GetPlayerCrouch()return crouch or 0 end
function GetPlayerTool()return selected end
function SetPlayerTool(t)selected=t end
function GetToolBody()return selected=="vrhands" and 0 or 1 end
function GetShapeVoxelCount()return 12 end
function GetShapeLocalTransform()return Transform()end
function CreateShape(body,t,ref)serial=serial+1;objects[serial]={kind="shape",body=body,pose=t};table.insert(objects[body].shapes,serial);return serial end
function CopyShapeContent()end
function CopyShapePalette()end
function Delete(h)objects[h]=nil end
function GetTime()return now end
function GetShapeBounds()return Vec(-.02,0,-.02),Vec(.02,.02,.02)end
function GetShapeClosestPoint(s,p)return true,p end
function QueryRejectBody()end
function GetLocalPlayer()return 1 end
function QueryRejectPlayer()end
function QueryRaycast()return hitEnabled or false,.01,Vec(-1,0,0),2 end
function GetShapeBody()return 3 end
function GetShapeMaterialAtPosition()return material or "metal"end
function MakeHole(at,soft,medium,hard)damage=damage+1;lastRadius=soft;assert(soft<=.30 and medium<=.18 and hard<=.10)end
function IsBodyDynamic()return true end
function ApplyBodyImpulse()end
objects[1]={kind="body",pose=Transform(),shapes={2}};objects[2]={kind="shape",body=1};objects[3]={kind="body",pose=Transform(),shapes={}}
function step()now=now+.016;tick(.016)end
function hand(h,x,y,z) local i=h==1 and 2 or 9;sample[i]=x;sample[i+1]=y;sample[i+2]=z end
''')
lua.execute(Path('mods/sledge-vr-accessories/main.lua').read_text())
checks=[
 ('creates docked voxel tablet','step();assert(tabletBody and report[3]==-1)'),
 ('one proximity pulse, no held repeat','hand(1,-.34,.65,0);step();local n=pulses[1];assert(n==1);step();step();assert(pulses[1]==n)'),
 ('one confirmed grab pulse and left-hand ownership','sample[25]=1;step();assert(report[1]==1 and report[3]==0 and pulses[1]==2);step();assert(pulses[1]==2)'),
 ('held tablet follows hand','hand(1,0,1.5,-.4);step();assert(math.abs(objects[tabletBody].pose.pos[3]+.4)<.001)'),
 ('release creates dynamic dropped tablet','sample[25]=0;step();assert(report[3]==-1 and objects[tabletBody].dynamic)'),
 ('walking away returns tablet to left thigh','sample[16]=4;step();assert(not objects[tabletBody].dynamic and math.abs(objects[tabletBody].pose.pos[1]-3.66)<.001)'),
 ('holster only after releasing grip at shoulder','sample[16]=0;hand(2,.22,1.45,.23);sample[26]=1;step();assert(selected=="sledge");sample[26]=0;step();step();assert(selected=="vrhands" and report[2]==1)'),
 ('stored inventory forces hands only','selected="gun";step();assert(selected=="vrhands")'),
 ('emergency restore unlocks original tool','emergency=1;step();assert(selected=="sledge" and report[2]==0)'),
 ('either shoulder can store and retrieve','hand(2,-.22,1.45,.23);sample[26]=1;step();sample[26]=0;step();step();assert(selected=="vrhands");hand(1,-.22,1.45,.23);sample[25]=1;step();assert(selected=="sledge" and report[2]==0);sample[25]=0'),
 ('walking translation does not damage','selected="vrhands";hitEnabled=true;hand(1,0,1,0);hand(2,1,1,0);step();local n=damage;for i=1,10 do sample[16]=sample[16]+.05;sample[2]=sample[2]+.05;sample[9]=sample[9]+.05;step() end;assert(damage==n)'),
 ('fast hand strike damages with bounded radius','local n=damage;sample[2]=sample[2]+.08;step();assert(damage==n+1 and lastRadius<.10)'),
 ('held contact cannot repeatedly damage within cooldown','local n=damage;for i=1,4 do sample[2]=sample[2]+.08;step() end;assert(damage==n)'),
 ('tracking discontinuity suppresses damage','local n=damage;sample[31]=sample[31]+1;sample[2]=sample[2]+3;step();assert(damage==n)'),
 ('melee switch blocks new strikes','sample[33]=0;local n=damage;for i=1,20 do sample[2]=sample[2]+.08;step() end;assert(damage==n)'),
 ('tablet stays at left thigh after 180 degree turn','playerRotation=Quat(0,1,0,0);sample[16]=10;step();local p=objects[tabletBody].pose.pos;assert(math.abs(p[1]-10.34)<.001 and math.abs(p[2]-.65)<.001 and math.abs(p[3])<.001);assert(not objects[tabletBody].dynamic)'),
 ('crouching follows thigh height','crouch=1;step();assert(math.abs(objects[tabletBody].pose.pos[2]-.40)<.001);crouch=0'),
 ('attached tablet never participates in player collision queries','for _,shape in ipairs(objects[tabletBody].shapes)do assert(objects[shape].layer==0 and objects[shape].mask==0)end'),
 ('native hammer swing dispatches without synthetic tool damage',r'''
 sample[33]=1;sample[34]=1000;sample[16]=0;playerRotation=nil;hand(1,-2,1,0);hand(2,0,1,0);sample[22]=1;sample[25]=0;sample[26]=0
 selected="sledge";step()
 function swingStep(dx,elapsed)sample[9]=sample[9]+dx;sample[34]=sample[34]+(elapsed or 16);step()end
 swingStep(0);swingStep(0);local a,n=attacks,damage;swingStep(.08);assert(attacks==a+1 and damage==n)
 for i=1,5 do swingStep(.08)end;assert(attacks==a+1)
 now=now+1;swingStep(0);swingStep(.08);assert(attacks==a+2)
 '''),
 ('walking with melee tool does not attack','now=now+1;swingStep(0);local n=attacks;for i=1,5 do sample[16]=sample[16]+.05;swingStep(.05)end;assert(attacks==n)'),
 ('unknown mod primary attack can be enabled per tool','selected="mod_opaque_123";step();swingStep(0);swingStep(0);local n=attacks;swingStep(.08);assert(attacks==n);modeCycle=1;step();swingStep(0);swingStep(0);now=now+1;swingStep(.08);assert(attacks==n+1 and currentMode==1)'),
 ('per-tool OFF suppresses physical attack','modeCycle=1;step();swingStep(0);swingStep(0);now=now+1;local n=attacks;swingStep(.08);assert(attacks==n and currentMode==2)'),
 ('recognized mod hammer uses its own primary attack','selected="mod_super_hammer";step();swingStep(0);swingStep(0);now=now+1;local n=attacks;swingStep(.08);assert(attacks==n+1)'),
 ('menus and vehicles suppress swings','local n=attacks;sample[30]=1;swingStep(.08);sample[30]=0;sample[29]=1;swingStep(.08);sample[29]=0;assert(attacks==n)'),
 ('tracking jumps cannot attack','swingStep(0);swingStep(0);now=now+1;local n=attacks;swingStep(2);assert(attacks==n)'),
 ('new controller sample time tolerates moderate FPS drops','swingStep(0);swingStep(0);now=now+1;local n=attacks;swingStep(.3,100);assert(attacks==n+1)'),
 ('VR disable cleans accessory and inventory state','enabled=false;step();assert(not objects[tabletBody] and report[2]==0)'),
]
for label,code in checks:
 lua.execute(code)
 print('PASS',label)
print(f'{len(checks)} companion behavior checks passed (mock engine; Lua 5.1).')
# Test only the VR additions; no proprietary game script is distributed.
import json
patches=json.loads(Path('package/patches/game-scripts.json').read_text())
native='\n'.join(h['after'] for p in patches if p['name']=='spawn' for h in p['hunks'])
start=native.index('-- SLEDGE_TABLET_BEGIN');end=native.index('-- SLEDGE_TABLET_END')
menu=LuaRuntime(unpack_returned_tuples=True)
menu.execute('''protocol=717;vr=1;request=0;page=2;spawnUi=false;spawnPlacement=false;allowed=true
function GetPlayerHealth(_,op,a,b) if op==0 then return protocol,vr elseif op==7 then local r=request;request=0;return r elseif op==5 then return page elseif op==6 then page=a elseif op==8 then keyboard=b;shown=a end end
function spawnEnabled() return allowed end
function spawnAbort() aborted=true end
''')
menu.execute('function update() '+native[start:end]+' end')
menu.execute('request=1;spawnPlacement=true;update();assert(spawnUi and aborted and gSledgeTabletOwnsSpawn and request==0)')
menu.execute('gSledgeKeyboard=true;update();update();assert(keyboard==1)')
menu.execute('request=2;update();assert(not gSledgeKeyboard and keyboard==0)')
menu.execute('page=0;update();assert(not spawnUi and not gSledgeTabletOwnsSpawn)')
menu.execute('allowed=false;page=2;request=1;update();assert(page==0 and not spawnUi)')
menu.execute('protocol=1;request=1;spawnUi=false;update();assert(request==1 and not spawnUi)')
print('PASS native spawn open, placement cancellation, persistent keyboard, Enter, close, disabled spawning and no-DLL behavior')
