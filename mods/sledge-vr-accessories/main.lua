-- Legacy single-player mod: all destructive work runs on the game update thread.
-- No level-wide object scans. At most one holster shape is copied per frame.
local function bridge(op, ...) return GetPlayerHealth(-717, op, ...) end
local tablet, held, docked, gripPose = 0, 0, true, nil
local previousGrip, near = {false,false}, {false,false}
local holsterNear = {false,false}
local holster, copyJob, storedTool, shoulder = 0, nil, nil, 1
local lastHand, lastHead, lastCenter, lastTool = {}, nil, nil, nil
local strikes, cooldown, probes = {}, {0,0}, {}
local lastReady, nextSpawn = false, 0
local meleeStamp, probeGrip = nil, nil
local function valid(h) return h and h ~= 0 and IsHandleValid(h) end
local function distance(a,b) return VecLength(VecSub(a,b)) end
local function pulse(h) bridge(3,h-1) end
local function pose(t,i) return Transform(Vec(t[i],t[i+1],t[i+2]),Quat(t[i+3],t[i+4],t[i+5],t[i+6])) end
local function offset(base,x,y,z) return TransformToParentTransform(base,Transform(Vec(x,y,z))) end
local function accessory(body)
    SetTag(body,"sledge_accessory");SetTag(body,"unbreakable");SetBodyDynamic(body,false);SetBodyVelocity(body,Vec());SetBodyAngularVelocity(body,Vec())
    for _,s in ipairs(GetBodyShapes(body)) do SetTag(s,"unbreakable");SetShapeCollisionFilter(s,0,0) end
end
local function spawnBody(xml,transform)
    for _,h in ipairs(Spawn(xml,transform,true)) do if GetEntityType(h)=="body" then accessory(h);return h end end
    return 0
end
local function pin(body,transform)
    SetBodyDynamic(body,false);SetBodyVelocity(body,Vec());SetBodyAngularVelocity(body,Vec());SetBodyTransform(body,transform)
end
local function restore()
    if storedTool then SetPlayerTool(storedTool) end
    if valid(holster) then Delete(holster) end
    if copyJob and valid(copyJob.body) then Delete(copyJob.body) end
    holster=0;copyJob=nil;storedTool=nil;bridge(4)
end
local function beginHolster(side,tool,hand)
    local source=GetToolBody();if not valid(source) or tool=="vrhands" or tool=="" then return end
    local shapes=GetBodyShapes(source);if #shapes==0 or #shapes>64 then return end
    local count=0;for _,s in ipairs(shapes) do count=count+GetShapeVoxelCount(s) end
    if count>150000 then return end -- do not freeze a frame copying enormous mod tools
    local body=spawnBody('<body dynamic="false" tags="sledge_accessory invisible"><voxbox size="1 1 1"/></body>',GetBodyTransform(source))
    if not valid(body) then return end
    copyJob={body=body,source=source,shapes=shapes,index=1,tool=tool,side=side,placeholder=GetBodyShapes(body)[1],grip=TransformToLocalTransform(GetBodyTransform(source),hand)}
end
local function copyHolster()
    local j=copyJob;if not j then return end
    if not valid(j.source) or GetPlayerTool()~=j.tool then Delete(j.body);copyJob=nil;return end
    local src=j.shapes[j.index]
    if not valid(src) then Delete(j.body);copyJob=nil;return end
    local dst=CreateShape(j.body,TransformToLocalTransform(j.grip,GetShapeLocalTransform(src)),src)
    if not valid(dst) then Delete(j.body);copyJob=nil;return end
    CopyShapeContent(src,dst);CopyShapePalette(src,dst);SetShapeCollisionFilter(dst,0,0);SetTag(dst,"unbreakable")
    j.index=j.index+1
    if j.index>#j.shapes then
        if valid(j.placeholder) then Delete(j.placeholder) end
        RemoveTag(j.body,"invisible")
        holster=j.body;storedTool=j.tool;shoulder=j.side;copyJob=nil
        SetPlayerTool("vrhands");pulse(2)
    end
end
local function reject(tool)
    if valid(tool) then QueryRejectBody(tool) end
    if valid(tablet) then QueryRejectBody(tablet) end
    if valid(holster) then QueryRejectBody(holster) end
    QueryRejectPlayer(GetLocalPlayer())
end
local strengths={metal=1,hardmetal=1,heavymetal=1,wood=0.60,hardwood=0.65,plastic=0.18,
    rope=0,foliage=0,paper=0.08,glass=0.25,masonry=0.80,rock=0.80,plaster=0.3}
local function buildProbes(tool,hand)
    probes={};probeGrip=nil;if not valid(tool) then return end
    local body=GetBodyTransform(tool);local shapes=GetBodyShapes(tool);probeGrip=TransformToLocalTransform(hand,body)
    -- Eight surface probes per shape, at most four shapes: bounded to 32 rays.
    for i=1,math.min(#shapes,4) do
        local s=shapes[i];local lo,hi=GetShapeBounds(s)
        for corner=0,7 do
            local p=Vec(corner%2==0 and lo[1] or hi[1],math.floor(corner/2)%2==0 and lo[2] or hi[2],corner<4 and lo[3] or hi[3])
            local ok,point=GetShapeClosestPoint(s,p)
            if ok then
                table.insert(probes,{localPoint=TransformToLocalPoint(body,point),shape=s,material=GetShapeMaterialAtPosition(s,point,true)})
            end
        end
    end
end
-- Tools use their own normal primary attack; no synthetic voxel damage.
local swingTool, swingBody, swingMode = nil, nil, 0
local swingPrevious, swingHead, swingCenter = {}, nil, nil
local swingArmed, swingNext, swingAllowed = true, 0, false
local function resetSwing()
    swingPrevious={};swingHead=nil;swingArmed=true
end
local meleeNames={sledge=true,hammer=true,sledgehammer=true,axe=true,hatchet=true,
    crowbar=true,sword=true,katana=true,knife=true,machete=true,bat=true,club=true,
    pickaxe=true,shovel=true,wrench=true,fists=true,fist=true,melee=true}
local function recognizesMelee(id,tool)
    if id=="vrhands" or id=="" then return false end
    if valid(tool) and HasTag(tool,"sledge_vr_melee") then return true end
    -- Whole words avoid auto-firing unrelated weapons with similar substrings.
    local label=id.." "..GetString("game.tool."..id..".name")
    for word in string.lower(label):gmatch("%a+") do if meleeNames[word] then return true end end
    return false
end
local function nativeSwing(hands,head,dt,s,tool)
    local id=GetPlayerTool()
    if id~=swingTool or tool~=swingBody then
        swingTool=id;swingBody=tool;swingMode=GetInt("savegame.mod.sledgevr.swing."..id)
        swingAllowed=recognizesMelee(id,tool);buildProbes(tool,hands[2]);resetSwing()
    end
    local cycle=bridge(11,swingMode) or 0
    if cycle>0 then
        swingMode=(swingMode+cycle)%3;SetInt("savegame.mod.sledgevr.swing."..id,swingMode)
        bridge(11,swingMode);resetSwing()
    end
    local allowed=swingMode==1 or (swingMode==0 and swingAllowed)
    if not allowed or s[33]~=1 or s[24]~=1 or s[29]~=0 or s[30]~=0 or held>0 or storedTool or s[32]~=0 or id=="vrhands" or not valid(tool) then resetSwing();return end
    if dt<=0 or dt>0.15 or not swingHead or swingCenter~=s[31] or distance(head.pos,swingHead)>0.5 then
        resetSwing();swingHead=VecCopy(head.pos);swingCenter=s[31];return
    end
    local translation=VecSub(head.pos,swingHead);swingHead=VecCopy(head.pos)
    local toolPose=probeGrip and TransformToParentTransform(hands[2],probeGrip)
    if not toolPose then resetSwing();return end
    local maxSpeed,discontinuous=0,false
    for n,probe in ipairs(probes) do
        local p=TransformToParentPoint(toolPose,probe.localPoint);local old=swingPrevious[n];swingPrevious[n]=VecCopy(p)
        if old then
            local motion=VecSub(VecSub(p,old),translation);local length=VecLength(motion)
            if length>0.75 then discontinuous=true end
            maxSpeed=math.max(maxSpeed,length/dt)
        end
    end
    if discontinuous or maxSpeed>30 then resetSwing();return end
    if maxSpeed<0.65 then swingArmed=true end
    if maxSpeed>=1.6 and swingArmed and GetTime()>=swingNext then
        if bridge(10)==1 then pulse(2);swingArmed=false;swingNext=GetTime()+0.30 end
    end
end
local function melee(hands,head,dt,s,tool)
    local changed=lastCenter~=s[31] or tool~=lastTool
    if tool~=lastTool then lastTool=tool end
    if dt<=0 or dt>0.15 or changed or not lastHead or distance(head.pos,lastHead)>0.5 then strikes={};lastHead=VecCopy(head.pos);lastCenter=s[31];return end
    local translation=VecSub(head.pos,lastHead);lastHead=VecCopy(head.pos)
    local toolPose=valid(tool) and probeGrip and TransformToParentTransform(hands[2],probeGrip) or nil
    for h=1,2 do
        local list=(h==2 and GetPlayerTool()~="vrhands") and {} or {{localPoint=Vec(),bare=true}}
        for n,probe in ipairs(list) do
            local p=probe.bare and hands[h].pos or TransformToParentPoint(toolPose,probe.localPoint)
            local key=h*100+n;local old=strikes[key];strikes[key]=VecCopy(p)
            if old and s[22+h]==1 and held~=h and s[29]==0 and s[30]==0 and GetTime()>cooldown[h] then
                local delta=VecSub(p,old);local motion=VecSub(delta,translation);local speed=VecLength(motion)/dt
                local length=VecLength(delta)
                if speed>1.0 and speed<30 and length>0.001 and length<1.0 then
                    reject(tool);local hit,d,norm,shape=QueryRaycast(old,VecNormalize(delta),length+0.012,0.008)
                    if hit and VecDot(VecNormalize(motion),norm)<-0.2 then
                        local at=VecAdd(old,VecScale(VecNormalize(delta),d));local body=GetShapeBody(shape)
                        local material=probe.bare and "hands" or probe.material
                        local strength=probe.bare and 0.32 or (strengths[material] or 0.18)
                        if strength>0 then
                            local energy=math.min(1,(speed-1.0)/5)*strength
                            if energy>0.03 then
                                MakeHole(VecSub(at,VecScale(norm,0.012)),0.30*energy,0.18*energy,(material=="metal" or material=="hardmetal" or material=="heavymetal") and 0.10*energy or 0,true)
                                if IsBodyDynamic(body) then ApplyBodyImpulse(body,at,VecScale(VecNormalize(motion),math.min(8,energy*8))) end
                                pulse(h);cooldown[h]=GetTime()+0.15
                            end
                        end
                    end
                end
            end
        end
    end
end
function tick(dt)
    local protocol,on=bridge(0)
    if protocol~=717 or on~=1 then
        if lastReady then restore();if valid(tablet) then Delete(tablet) end;tablet=0;held=0 end
        lastReady=false;return
    end
    local s={bridge(1)};if s[1]~=717 then strikes={};lastHand={};lastHead=nil;return end
    lastReady=true
    local hands={pose(s,2),pose(s,9)};local head=pose(s,16)
    -- Belt and shoulder sockets belong to the avatar, not the rendered camera
    -- (which can be replaced temporarily by tool aiming or vehicle rendering).
    local base=GetPlayerTransform();local crouch=math.max(0,math.min(1,GetPlayerCrouch()))
    local dock=offset(base,-0.34,0.65-crouch*0.25,0)
    dock.rot=QuatRotateQuat(base.rot,QuatEuler(0,-90,0))
    local shoulders={offset(base,-0.22,1.45-crouch*0.45,0.23),offset(base,0.22,1.45-crouch*0.45,0.23)}
    local down={s[25]==1,s[26]==1};local tracked={s[23]==1,s[24]==1}
    if not valid(tablet) and GetTime()>=nextSpawn then
        nextSpawn=GetTime()+5;tablet=spawnBody("MOD/tablet.xml",dock);docked=true;held=0
        if valid(tablet) then
            -- Center imported geometry once, independent of the VOX origin.
            local lo,hi=GetBodyBounds(tablet);local center=TransformToLocalPoint(dock,VecScale(VecAdd(lo,hi),0.5))
            for _,shape in ipairs(GetBodyShapes(tablet)) do local t=GetShapeLocalTransform(shape);t.pos=VecSub(t.pos,center);SetShapeLocalTransform(shape,t) end
            DebugPrint("Sledge r7.20 left thigh tablet: "..VecStr(dock.pos))
        end
    end
    if valid(tablet) then
        if docked then pin(tablet,dock);for _,shape in ipairs(GetBodyShapes(tablet)) do SetShapeCollisionFilter(shape,0,0) end end
        for h=1,2 do
            local close=false
            if tracked[h] and held==0 then local ok,p=GetBodyClosestPoint(tablet,hands[h].pos);close=ok and distance(p,hands[h].pos)<0.075 end
            if close and not near[h] then pulse(h) end
            if not close then
                if distance(hands[h].pos,GetBodyTransform(tablet).pos)>0.30 then near[h]=false end
            else near[h]=true end
            local occupied=math.floor(s[32]/2^(h-1))%2==1
            if close and not occupied and down[h] and not previousGrip[h] and (h==1 or GetPlayerTool()=="vrhands") then
                held=h;docked=false;gripPose=TransformToLocalTransform(hands[h],GetBodyTransform(tablet));SetBodyDynamic(tablet,false);pulse(h)
            end
        end
        if held>0 then
            if not tracked[held] or (not down[held] and s[30]==0) then
                SetBodyDynamic(tablet,true)
                for _,shape in ipairs(GetBodyShapes(tablet)) do SetShapeCollisionFilter(shape,1,255) end
                local velocity=Vec();if lastHand[held] and dt>0 and dt<0.05 then velocity=VecScale(VecSub(hands[held].pos,lastHand[held]),1/dt);if VecLength(velocity)>8 then velocity=VecScale(VecNormalize(velocity),8) end end
                SetBodyVelocity(tablet,velocity);held=0;bridge(6,0)
            else
                for _,shape in ipairs(GetBodyShapes(tablet)) do SetShapeCollisionFilter(shape,0,0) end
                pin(tablet,TransformToParentTransform(hands[held],gripPose))
            end
        elseif distance(GetBodyTransform(tablet).pos,dock.pos)>1.5 then
            docked=true;SetBodyDynamic(tablet,false);SetBodyVelocity(tablet,Vec());SetBodyTransform(tablet,dock)
            for _,shape in ipairs(GetBodyShapes(tablet)) do SetShapeCollisionFilter(shape,0,0) end
        end
    end
    copyHolster()
    if storedTool then
        if not valid(holster) then restore() else
            pin(holster,shoulders[shoulder])
            SetPlayerTool("vrhands")
            for h=1,2 do
                local d=distance(hands[h].pos,shoulders[shoulder].pos)
                if tracked[h] and d<0.20 and not holsterNear[h] then pulse(h);holsterNear[h]=true end
                if d>0.30 then holsterNear[h]=false end
                if tracked[h] and down[h] and not previousGrip[h] and d<0.20 then restore();pulse(h);break end
            end
        end
    elseif not copyJob and held~=2 and previousGrip[2] and not down[2] and s[29]==0 and s[30]==0 then
        for side=1,2 do if distance(hands[2].pos,shoulders[side].pos)<0.22 then beginHolster(side,GetPlayerTool(),hands[2]);break end end
    end
    local transform=valid(tablet) and GetBodyTransform(tablet) or Transform();local g=gripPose or Transform()
    local emergency=bridge(2,held>0 and 2^(held-1) or 0,storedTool and 1 or 0,held-1,
        transform.pos[1],transform.pos[2],transform.pos[3],transform.rot[1],transform.rot[2],transform.rot[3],transform.rot[4],
        g.pos[1],g.pos[2],g.pos[3],g.rot[1],g.rot[2],g.rot[3],g.rot[4])
    if emergency==1 then restore() end
    local elapsed=dt
    if s[34] and meleeStamp then elapsed=(s[34]-meleeStamp)%1000000/1000 end
    meleeStamp=s[34]
    nativeSwing(hands,head,elapsed,s,GetToolBody())
    if s[33]==1 then melee(hands,head,elapsed,s,GetToolBody()) else strikes={};lastHead=nil end
    previousGrip=down;lastHand={VecCopy(hands[1].pos),VecCopy(hands[2].pos)}
end
