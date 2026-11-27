-- Weapon patch r7.34: original hYve bullets with buffered taps and complete shot volleys.
-- hYve mYnd weapon behavior adapted from MythRillin / SnakeyWakey.
-- No ragdoll, possession, locomotion, global camera or AI code.
local guns={
 {id="hyve_glock",name="hYve Glock-18",file="glock",grip={-0.08,-0.065,0.265},sound="silencedglock",interval=.2,radius=.1,pellets=1},
 {id="hyve_g36",name="hYve G36K",file="g36",grip={0.13,0.57,-0.40},sound="g36",interval=.1,radius=.1,pellets=1},
 {id="hyve_magnum",name="hYve Magnum",file="magnum",grip={0.12,0.03,-0.41},sound="revolver",interval=.2,radius=.5,pellets=6},
 {id="hyve_shotgun",name="hYve Shotgun",file="shotgun",grip={0,0.05,0.79},sound="shotgun",interval=.8,radius=.15,pellets=65,spread=4},
 {id="hyve_flame",name="hYve Flamethrower",file="flame",grip={-0.1,0.08,0.35},sound="blow",interval=.03,flame=true,pellets=1}
}
local byId,saved={},{}
local packWas,onlyWas=false,false
local nextInventory,nextShot,nextSound=0,0,0
local queue=nil
local previousTool=nil
local flashUntil=0
local flashSprite=0
local laserDot=0
local toggleOnSound,toggleOffSound=0,0
local activeLight=0
local secondaryReady=false
local fireWasDown=false
local pendingUntil=0
local sightBody,sightShape=0,0
local function bridge(op,...) return GetPlayerHealth(-717,op,...) end
local function valid(h) return h and h~=0 and IsHandleValid(h) end
local function lightsOff()
 if valid(activeLight) then SetLightEnabled(activeLight,false) end
 activeLight=0
end
local function cancelEffects() lightsOff();flashUntil=0;secondaryReady=false end
local function cancelFire() queue=nil;pendingUntil=0;fireWasDown=false end
local function updateSight(g,body)
 if g.file~="glock" then sightBody=0;sightShape=0;return end
 if sightBody~=body then
  sightBody=body;sightShape=0
  local shapes=GetBodyShapes(body)
  for i=1,math.min(#shapes,8) do
   if HasTag(shapes[i],"vr_sight_glow") then sightShape=shapes[i];break end
  end
 end
 -- Equipped tools may reset their emissive multiplier. Restore only the
 -- tagged Glock shape; its four green insert voxels supply the soft glow.
 if valid(sightShape) then SetShapeEmissiveScale(sightShape,.8) end
end
local function updateAttachment(g,tool,muzzle,body,on,now)
 if not on or (g.file~="glock" and g.file~="g36") then lightsOff();return end
 if g.file=="g36" then
  lightsOff()
  local direction=TransformToParentVec(tool,Vec(0,0,-1))
  local origin=TransformToParentPoint(muzzle,Vec(.1,-.015,-.06))
  QueryRejectBody(body);QueryRejectPlayer(GetLocalPlayer())
  local hit,distance,normal=QueryRaycast(origin,direction,80)
  -- Only a surface dot: no beam geometry, red cone or world illumination.
  if hit and normal and distance and distance>=0 and distance<=80 then
   local point=VecAdd(VecAdd(origin,VecScale(direction,distance)),VecScale(normal,.004))
   DrawSprite(laserDot,Transform(point,QuatLookAt(point,VecAdd(point,normal))),.12,.12,4,0,0,1,true,true)
  end
  return
 end
 -- Preserve the Glock's white cone light.
 -- A standalone light avoids the tool shape's emissive scaling/disabled state.
 -- Its lens sits beyond the barrel geometry to prevent self-shadowing.
 local offset=Vec(0,-.035,-.15)
 local lens=Transform(TransformToParentPoint(muzzle,offset),
                      QuatRotateQuat(tool.rot,QuatEuler(0,180,0)))
 if not valid(g.light) then
  if now<(g.lightRetry or 0) then return end
  g.lightRetry=now+2 -- failed spawn never retries every frame
  local xml='<light type="cone" color="0.81 0.9 1" scale="100" reach="30" angle="15" size="0.04" glare="0"/>'
  local entities=Spawn(xml,lens,true)
  for _,h in ipairs(entities) do
   if GetEntityType(h)=="light" then g.light=h;break end
  end
  if not valid(g.light) then return end
 end
 SetProperty(g.light,"transform",lens)
 if activeLight~=g.light then
  lightsOff();SetLightEnabled(g.light,true);activeLight=g.light
 end
end
local function muzzleFlash(muzzle,now)
 if now>=flashUntil then return end
 local fade=math.min(1,(flashUntil-now)/.025)
 -- Three small world-space quads, attached to this frame's muzzle. Native
 -- sprite rendering supplies depth testing, additive light and both eye views.
 for _,rot in ipairs({Quat(),QuatEuler(90,0,0),QuatEuler(0,90,0)}) do
  DrawSprite(flashSprite,Transform(muzzle.pos,QuatRotateQuat(muzzle.rot,rot)),.14,.14,5,2,.2,fade,true,true)
 end
 PointLight(muzzle.pos,1,.49,0,2*fade)
end
local function toolEnabled(id,on) SetBool("game.tool."..id..".enabled",on) end
local function restoreTools()
 for id,on in pairs(saved) do toolEnabled(id,on) end
 saved={}
end
function init()
 toggleOnSound=LoadSound("clickdown.ogg");toggleOffSound=LoadSound("clickup.ogg")
 flashSprite=LoadSprite("gfx/laser.png");laserDot=LoadSprite("gfx/glare.png")
 for _,g in ipairs(guns) do
  RegisterTool(g.id,g.name,"MOD/"..g.file..".xml",6)
  toolEnabled(g.id,false);g.snd=LoadSound("MOD/sounds/"..g.sound..".ogg");byId[g.id]=g
 end
end
local function inventory(on,only,now)
 if onlyWas and not only then restoreTools() end
 if on~=packWas then
  if not on and byId[GetPlayerTool()] then SetPlayerTool("vrhands") end
  for _,g in ipairs(guns) do toolEnabled(g.id,on) end
 end
 if only and (not onlyWas or now>=nextInventory) then
  nextInventory=now+.5
  local keys=ListKeys("game.tool")
  -- Bounded registry inventory, never a world-object scan.
  for i=1,math.min(#keys,512) do
   local id=keys[i]
   if id~="vrhands" and not byId[id] then
    local key="game.tool."..id..".enabled"
    local enabled=GetBool(key)
    if saved[id]==nil then saved[id]=enabled elseif enabled then saved[id]=true end
    if enabled then toolEnabled(id,false) end
   end
  end
  local selected=GetPlayerTool()
  if selected~="vrhands" and not byId[selected] then SetPlayerTool("vrhands") end
 end
 packWas=on;onlyWas=only
end
local function shot(g,origin,direction,body)
 QueryRejectBody(body);QueryRejectPlayer(GetLocalPlayer())
 local range=g.flame and 4 or 150
 local hit,distance,normal=QueryRaycast(origin,direction,range)
 local point=VecAdd(origin,VecScale(direction,distance or range))
 if hit then
  if g.flame then SpawnFire(point)
  else
   -- Keep the source hYve damage sizes. Place the cut just inside the hit
   -- surface so floating-point rounding cannot leave its center in air.
   local impact=normal and VecSub(point,VecScale(normal,.005)) or point
   MakeHole(impact,g.radius,g.radius)
  end
 end
 ParticleReset();ParticleType("plain");ParticleColor(1,.2,0);ParticleRadius(g.flame and .12 or .025,0)
 ParticleAlpha(1,0);ParticleCollide(0);ParticleEmissive(5)
 SpawnParticle(origin,VecScale(direction,g.flame and 8 or 80),g.flame and .35 or .15)
end
function tick(dt)
 local protocol,enabled,restricted=bridge(12)
 local on=protocol==723 and enabled==1
 local only=on and restricted==1
 local now=GetTime();inventory(on,only,now)
 if not on then cancelFire();previousTool=nil;cancelEffects();return end
 local selected=GetPlayerTool();local g=byId[selected]
 if selected~=previousTool then cancelFire();nextShot=now;previousTool=selected;cancelEffects() end
 local s={bridge(1)}
 -- Trigger is the tracked right trigger. Head/camera direction is never used.
 if not g or s[1]~=717 or s[24]~=1 or s[29]==1 or s[30]==1 or math.floor((s[32] or 0)/2)%2==1 then cancelFire();cancelEffects();return end
 SetToolTransform(Transform(),0)
 SetToolHandPoseLocalTransform(Transform(Vec(unpack(g.grip)),QuatEuler(0,90,0)),nil)
 local body=GetToolBody();if not valid(body) then cancelFire();cancelEffects();return end
 updateSight(g,body)
 local tool=GetBodyTransform(body)
 -- Trigger must be released before a new press; gripping an object or merely
 -- holding the grip button does not qualify as supporting the equipped tool.
 if s[27]~=1 then secondaryReady=true
 elseif secondaryReady then
  secondaryReady=false
  if s[35]==1 and s[23]==1 and s[25]==1 and (g.file=="glock" or g.file=="g36") then
   g.lightOn=not g.lightOn;bridge(3,0)
   PlaySound(g.lightOn and toggleOnSound or toggleOffSound,tool.pos,.7,false)
  end
 end
 -- Retained r7.23 lacks the support-grip field: keep its original always-on
 -- attachment behavior. Current builds start OFF and use grip + left trigger.
 local muzzle=GetToolLocationWorldTransform("muzzle") or tool
 updateAttachment(g,tool,muzzle,body,s[35]==nil or g.lightOn==true,now)
 -- Tool placement and two-hand aiming are supplied by Sledge's existing hooks.
 local fireDown=s[28]==1
 if fireDown and not fireWasDown then pendingUntil=math.min(math.max(now,nextShot)+.12,now+1) end
 fireWasDown=fireDown
 if pendingUntil>0 and now>pendingUntil then pendingUntil=0 end
 if not queue and (fireDown or pendingUntil>0) and now>=nextShot then
  pendingUntil=0
  nextShot=now+g.interval -- no catch-up bursts after a stalled frame
  if not g.flame then flashUntil=now+.045 end
  queue={gun=g,origin=VecCopy(muzzle.pos),rotation=tool.rot,remaining=g.pellets,body=body}
  if not g.flame or now>=nextSound then PlaySound(g.snd,muzzle.pos);nextSound=now+.39 end
  bridge(3,1)
 end
 muzzleFlash(muzzle,now)
 if queue then
  -- Preserve the original 65-pellet shotgun while distributing work across ticks.
  for _=1,math.min(queue.remaining,8) do
   local rot=queue.rotation
   if queue.gun.spread then rot=QuatRotateQuat(rot,QuatEuler(math.random(-4,4),math.random(-4,4),math.random(-4,4))) end
   local direction=TransformToParentVec(Transform(Vec(),rot),Vec(0,0,-1))
   shot(queue.gun,queue.origin,direction,queue.body);queue.remaining=queue.remaining-1
  end
  if queue.remaining==0 then queue=nil end
 end
end
function shutdown()
 cancelFire();cancelEffects();restoreTools()
 for _,g in ipairs(guns) do
  if valid(g.light) then Delete(g.light) end
  g.light=nil;toolEnabled(g.id,false)
 end
end
