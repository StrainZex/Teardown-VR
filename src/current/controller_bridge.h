#pragma once
// Included inside the renderer bridge namespace after Log/Readable.
namespace ControllerBridge {
using Controllers::Transform;using Controllers::HandPose;
#include "haptics.h"
struct TagName {char data[32]{};explicit TagName(const char* text){strncpy_s(data,text,30);}};
struct Snapshot { glm::mat4 hand[2]{glm::mat4(1),glm::mat4(1)},head{1.f};bool valid[2]{};Controllers::Buttons buttons;ULONGLONG stamp=0;bool input=false;unsigned center=0; };
Snapshot latest;std::mutex mutex;
std::atomic<bool> separateGrab{true},inVehicle{false};
std::atomic<unsigned> heldObjects{0};
std::atomic<std::uint64_t> interactionTarget{0};
std::atomic<bool> physicalContact{true},handsOnly{false},physicalMelee{true},widerView{false};
std::mutex contactMutex;glm::vec3 contactOffset[2]{};ULONGLONG contactStamp=0;
using ToolFn=void(*)(void*,float);ToolFn originalTool=nullptr;
ToolFn originalPlayer=nullptr;
using CameraFn=Transform*(*)(void*);CameraFn originalCamera=nullptr;
using VisibilityFn=void(*)(void*,bool);VisibilityFn originalVisibility=nullptr;
using AimFn=void(*)(void*,glm::vec3*,glm::vec3*);AimFn originalAim=nullptr;
using BodyFn=void(*)(void*,const Transform*);BodyFn setBody=nullptr;
using HandsFn=void(*)(void*,const HandPose*,const HandPose*);HandsFn setHands=nullptr;
std::uint8_t* gameBase=nullptr;
std::atomic<float> vehicleTurnSpeed{90.f},vehicleHeight{0.f};
std::atomic<ULONGLONG> vehicleAdjustAt{0};
std::atomic<void*> vehicleAdjustTarget{nullptr};
std::atomic<float> referenceYaw{0},turnYaw{0};std::atomic<bool> headingReady{false},headFollow{true},menuOpen{false};
std::atomic<bool> recenterRequested{false};std::atomic<float> turnSpeed{90.f};
std::atomic<bool> mapRequested{false};
std::atomic<bool> pauseRequested{false};
std::mutex headMutex;glm::vec3 headAnchor(0),headAnchorCamera(0);ULONGLONG headAnchorStamp=0;
std::atomic<float> headForward{.10f},vehicleManualHeight{0.f},vehicleManualForward{0.f};
float nativeReferenceYaw=0;void* headingPlayer=nullptr;unsigned headingCenter=0;
bool Copy(Snapshot& s){thread_local SnapshotCache::Reader<Snapshot> cache;return cache.Read(mutex,latest,enabled.load(),GetTickCount64(),s);}
bool Focused(){DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);return pid==GetCurrentProcessId();}
void* LocalPlayer(){
 auto slot=gameBase+0x1c96e70;if(!Readable(slot,sizeof(void*)))return nullptr;
 auto game=*reinterpret_cast<std::uint8_t**>(slot);if(!Readable(game,0x16c))return nullptr;
 int index=*reinterpret_cast<int*>(game+0x168);
 // The local index and array access are confirmed by the current camera/aim getters.
 if(index<0||index>63)return nullptr;
 auto players=*reinterpret_cast<void***>(game+0xC0);if(!Readable(players,(index+1)*sizeof(void*)))return nullptr;
 return players[index];
}
glm::mat4 BasePose(const glm::mat4& camera){
 auto base=Stereo::Upright(camera);
 if(headFollow&&headingReady&&!inVehicle){base=glm::rotate(glm::mat4(1.f),referenceYaw.load()+turnYaw.load(),glm::vec3(0,1,0));base[3]=camera[3];}
 {
  std::lock_guard<std::mutex> lock(headMutex);
  if(headAnchorStamp&&GetTickCount64()-headAnchorStamp<250)base=Controllers::AvatarCamera(base,camera,Controllers::CurrentHeadAnchor(headAnchor,headAnchorCamera,glm::vec3(camera[3])),inVehicle);
 }
 base[3]-=glm::vec4(glm::normalize(glm::vec3(camera[2]))*headForward.load(),0.f);
 if(inVehicle){base[3].y+=vehicleHeight.load()+vehicleManualHeight.load();base[3]-=glm::vec4(glm::vec3(base[2])*vehicleManualForward.load(),0.f);}
 return base;
}
bool WorldHands(void* player,const Snapshot& s,glm::mat4& left,glm::mat4& right,bool contacts=true){
 if((!s.valid[0]&&!s.valid[1])||player!=LocalPlayer()||!Readable(player,0xAB0))return false;
 Transform camera;std::memcpy(&camera,static_cast<std::uint8_t*>(player)+0x640,sizeof(camera));if(!Controllers::Valid(camera))return false;
 auto base=BasePose(Controllers::Matrix(camera));left=base*s.hand[0];right=base*s.hand[1];
 {std::lock_guard<std::mutex> guard(contactMutex);if(contacts&&physicalContact&&contactStamp&&GetTickCount64()-contactStamp<250){left[3]+=glm::vec4(contactOffset[0],0);right[3]+=glm::vec4(contactOffset[1],0);}}return true;
}
HandPose boundGrip{};void* boundBody=nullptr;void* boundSelection=nullptr;int boundId=0;
HandPose AttachmentGrip(std::uint8_t* player){
 auto mgr=*reinterpret_cast<std::uint8_t**>(player+0x990);auto selected=Readable(mgr,0x38)?*reinterpret_cast<void**>(mgr+0x30):nullptr;
 auto currentBody=*reinterpret_cast<std::uint8_t**>(player+0x820);
 if(boundBody==currentBody&&Readable(currentBody,0x60)&&boundSelection==selected&&boundId==*reinterpret_cast<int*>(currentBody+0xC))return boundGrip;
 HandPose grip{};std::memcpy(&grip,player+0xA70,sizeof(grip));
 if(Controllers::UsableGrip(grip))return grip;
 // First-person prefabs often omit fp_action_rh. Resolve the tool's authored
 // third-person handle through the same getter used by the Lua animation API.
 auto tool=*reinterpret_cast<std::uint8_t**>(player+0x990);
 auto body=*reinterpret_cast<void**>(player+0x820);
 thread_local void* cachedBody=nullptr;thread_local void* cachedTool=nullptr;thread_local HandPose cachedGrip{};thread_local bool cached=false;
 if(cached&&cachedBody==body&&cachedTool==tool)return cachedGrip;
 cached=true;cachedBody=body;cachedTool=tool;cachedGrip={};
 if(Readable(tool,0x38)){
  auto prefab=*reinterpret_cast<void**>(tool+0x30);
  if(!prefab||Readable(prefab,0x108)){
   using LocationFn=bool(*)(void*,const TagName*,Transform*);
   auto location=reinterpret_cast<LocationFn>(gameBase+0xc0c30);
   for(const char* name:{"tp_action_rh","fp_action_rh"}){
    HandPose candidate{};TagName tag(name);
    candidate.used=location(player,&tag,&candidate.transform);
    if(Controllers::UsableGrip(candidate)){cachedGrip=candidate;return candidate;}
   }
  }
 }
 return {};
}
namespace Accessories { unsigned Occupied(); bool Locked(); }
namespace SupportGrip {
 std::atomic<bool> active{false};void* body=nullptr;int bodyId=0;unsigned center=0;
 bool feedbackLatched=false,blockedUntilRelease=false;glm::vec3 anchor{0};std::mutex lock;glm::mat4 aim{1.f};ULONGLONG stamp=0;
 void Reset(){active=false;body=nullptr;std::lock_guard<std::mutex> guard(lock);stamp=0;}
 glm::mat4 Aim(const glm::mat4& fallback){std::lock_guard<std::mutex> guard(lock);return active&&GetTickCount64()-stamp<150?aim:fallback;}
 glm::mat4 Apply(void* player,void* tool,const Snapshot& s,const glm::mat4& left,const glm::mat4& right,const glm::mat4& oneHand,const HandPose& grip){
  if(!s.buttons.leftGrab){feedbackLatched=false;blockedUntilRelease=false;}
  int id=*reinterpret_cast<int*>(static_cast<std::uint8_t*>(tool)+0xC);
  if(body!=tool||bodyId!=id||center!=s.center){Reset();body=tool;bodyId=id;center=s.center;}
  if(blockedUntilRelease||(Accessories::Occupied()&1)||handsOnly||!s.valid[0]||!s.buttons.leftGrab||!s.input||menuOpen||inVehicle||heldObjects.load()||!separateGrab){active=false;return oneHand;}
  if(!active){
   struct Bounds {glm::vec3 low,high;} box{};
   using BoundsFn=void*(*)(void*,Bounds*);reinterpret_cast<BoundsFn>(gameBase+0x2f2a0)(tool,&box);
   bool valid=true;for(int axis=0;axis<3;axis++)if(!std::isfinite(box.low[axis])||!std::isfinite(box.high[axis])||box.low[axis]>box.high[axis])valid=false;
   if(!valid)return oneHand;
   glm::vec3 touch=glm::clamp(glm::vec3(left[3]),box.low,box.high);
   if(glm::length(touch-glm::vec3(left[3]))>.12f)return oneHand;
   Transform old;std::memcpy(&old,static_cast<std::uint8_t*>(tool)+0x40,sizeof(old));if(!Controllers::Valid(old))return oneHand;
   anchor=glm::vec3(glm::inverse(Controllers::Matrix(old))*glm::vec4(touch,1));active=true;
  }
  auto primary=Controllers::UsableGrip(grip)?glm::vec3(grip.transform.x,grip.transform.y,grip.transform.z):glm::vec3(0);
  if(glm::length(glm::vec3(left[3])-glm::vec3(oneHand*glm::vec4(anchor,1)))>1.f){active=false;return oneHand;}
  auto line=glm::vec3(left[3])-glm::vec3(right[3]);auto authoredLine=glm::mat3(oneHand)*(anchor-primary);
  if(glm::length(line)<.08f||glm::length(authoredLine)<.04f||glm::dot(glm::normalize(line),glm::normalize(authoredLine))<-.8f){active=false;blockedUntilRelease=true;return oneHand;}
  auto result=ContactMath::TwoHand(oneHand,primary,anchor,glm::vec3(right[3]),glm::vec3(left[3]));
  if(!feedbackLatched){Haptics::Grab(0);feedbackLatched=true;}
  {std::lock_guard<std::mutex> guard(lock);aim=glm::mat4(glm::mat3(result)*glm::transpose(glm::mat3(oneHand))*glm::mat3(right));aim[3]=right[3];stamp=GetTickCount64();}
  return result;
 }
}
glm::mat4 ConstrainTool(void* body,const glm::mat4& desired);
struct ToolContext {void* body=nullptr;std::uint8_t* player=nullptr;glm::mat4 right{1.f},left{1.f},camera{1.f};Snapshot sample;HandPose grip{};Transform transform{};bool replaced=false;};
thread_local ToolContext* toolContext=nullptr;
void BodyHook(void* body,const Transform* transform){
 if(toolContext&&toolContext->body==body&&Readable(transform,sizeof(*transform))&&Controllers::Valid(*transform)){
  auto& c=*toolContext;
  c.grip=AttachmentGrip(c.player);
  // Cache the model basis once per equipped tool, before feedback from our pose.
  static void* basisBody=nullptr;static void* basisSelection=nullptr;static int basisId=0;static glm::mat3 basis(1.f);static HandPose fixedGrip{};
  auto manager=*reinterpret_cast<std::uint8_t**>(c.player+0x990);auto selection=Readable(manager,0x38)?*reinterpret_cast<void**>(manager+0x30):nullptr;int id=*reinterpret_cast<int*>(static_cast<std::uint8_t*>(body)+0xC);
  if(body!=basisBody||selection!=basisSelection||id!=basisId){basis=glm::transpose(glm::mat3(c.camera))*glm::mat3(Controllers::Matrix(*transform));fixedGrip=c.grip;basisBody=body;basisSelection=selection;basisId=id;}
  c.grip=fixedGrip;boundGrip=fixedGrip;boundBody=body;boundSelection=selection;boundId=id;auto desired=glm::mat4(glm::mat3(c.right)*basis);auto anchor=Controllers::UsableGrip(c.grip)?glm::vec3(c.grip.transform.x,c.grip.transform.y,c.grip.transform.z):glm::vec3(0);desired[3]=glm::vec4(glm::vec3(c.right[3])-glm::mat3(desired)*anchor,1);
  desired=SupportGrip::Apply(c.player,body,c.sample,c.left,c.right,desired,c.grip);
  c.transform=Controllers::Pack(ConstrainTool(body,desired));
  if(Controllers::Valid(c.transform)){setBody(body,&c.transform);c.replaced=true;
   static void* feedbackBody=nullptr;static void* feedbackSelection=nullptr;static int feedbackId=0;
   if(body!=feedbackBody||selection!=feedbackSelection||id!=feedbackId){feedbackBody=body;feedbackSelection=selection;feedbackId=id;if(!handsOnly)Haptics::Grab(1);}return;}
 }
 setBody(body,transform);
}
void ToolHook(void* player,float dt){
 Snapshot s;glm::mat4 left,right;if(!Copy(s)||!s.valid[1]||!WorldHands(player,s,left,right)){if(player==LocalPlayer())SupportGrip::Reset();originalTool(player,dt);return;}
 auto p=static_cast<std::uint8_t*>(player);auto body=*reinterpret_cast<void**>(p+0x820);
 Transform camera;std::memcpy(&camera,p+0x640,sizeof(camera));
 if(!Readable(body,0x60)||!Controllers::Valid(camera)){originalTool(player,dt);return;}
 HandPose authored[2]{};std::memcpy(authored,p+0xA70,sizeof(authored));
 ToolContext context;context.body=body;context.player=p;context.right=right;context.left=left;context.sample=s;context.camera=Controllers::Matrix(camera);context.grip=authored[0];
 auto previous=toolContext;toolContext=&context;originalTool(player,dt);toolContext=previous;
 if(!context.replaced)return;
 auto tool=Controllers::Matrix(context.transform);
 std::memcpy(p+0x860,&context.transform,sizeof(context.transform));
 static void* loggedBody=nullptr;if(body!=loggedBody){Log("Tracked tool changed: preserving authored model rotation; grip="+std::to_string(authored[0].used));loggedBody=body;}
}
using GetHandsFn=void(*)(void*,HandPose*,HandPose*);GetHandsFn originalGetHands=nullptr;
void GetHandsHook(void* player,HandPose* rightPose,HandPose* leftPose){
 originalGetHands(player,rightPose,leftPose);
 Snapshot s;glm::mat4 left,right;if(!Copy(s)||!WorldHands(player,s,left,right))return;
 auto p=static_cast<std::uint8_t*>(player);auto body=*reinterpret_cast<std::uint8_t**>(p+0x820);
 if(!Readable(body,0x5C))return;
 Transform transform;std::memcpy(&transform,body+0x40,sizeof(transform));if(!Controllers::Valid(transform))return;
 auto local=glm::inverse(Controllers::Matrix(transform));
 if(SupportGrip::active&&SupportGrip::body==body){left[3]=Controllers::Matrix(transform)*glm::vec4(SupportGrip::anchor,1);right=SupportGrip::Aim(right);}
 // Read the same tool-local handle used to place the tool this tick.
 // Keep the visible right hand on that handle even if the body moved since placement.
 if(!handsOnly&&s.valid[1]&&!(heldObjects.load()&2)){auto grip=AttachmentGrip(p);auto anchor=Controllers::UsableGrip(grip)?glm::vec3(grip.transform.x,grip.transform.y,grip.transform.z):glm::vec3(0);right[3]=Controllers::Matrix(transform)*glm::vec4(anchor,1.f);}
 if(s.valid[1]){rightPose->transform=Controllers::Pack(local*Controllers::AnimationHand(right));rightPose->used=true;}else rightPose->used=false;
 if(s.valid[0]){leftPose->transform=Controllers::Pack(local*Controllers::AnimationHand(left));leftPose->used=true;}else leftPose->used=false;
 static bool logged=false;if(!logged){Log("Animation hands use native wrist axes; tool attachment resolves authored handles");logged=true;}
}
// The game's 32-byte short string stores up to 30 characters inline; the last
// byte is zero for inline storage. These fixed tag names require no allocation.

using HasTagFn=bool(*)(void*,const TagName*);HasTagFn hasTag=nullptr;
using RemoveTagFn=void(*)(void*,const TagName*);RemoveTagFn originalRemoveTag=nullptr;
using SetTagFn=void(*)(void*,const TagName*,const TagName*);SetTagFn setTag=nullptr;
thread_local bool showingLocalBody=false;
void RemoveTagHook(void* entity,const TagName* tag){
 if(showingLocalBody&&Readable(tag,32)&&std::memcmp(tag->data,"invisible",10)==0){
  static const TagName head("head"),neck("neck"),empty("");
  auto ancestor=static_cast<std::uint8_t*>(entity);
  for(int depth=0;depth<16&&Readable(ancestor,0x28);depth++){
   bool isHead=hasTag(ancestor,&head);
   if(isHead||hasTag(ancestor,&neck)){
    if(isHead&&*reinterpret_cast<unsigned short*>(ancestor+8)==1){
     struct HeadBounds{glm::vec3 low,high;} bounds{};
     using BoundsFn=void*(*)(void*,HeadBounds*);
     reinterpret_cast<BoundsFn>(gameBase+0x2f2a0)(ancestor,&bounds);
     auto size=bounds.high-bounds.low;bool valid=true;
     for(int axis=0;axis<3;axis++)if(!std::isfinite(size[axis])||size[axis]<0||size[axis]>.8f)valid=false;
     if(valid){std::lock_guard<std::mutex> lock(headMutex);headAnchor=(bounds.low+bounds.high)*.5f;auto local=static_cast<std::uint8_t*>(LocalPlayer());if(Readable(local,0x64C)){std::memcpy(&headAnchorCamera,local+0x640,12);headAnchorStamp=GetTickCount64();}}
    }
    setTag(entity,tag,&empty);
    static bool logged=false;if(!logged){Log("Local head/neck hidden in VR; body remains visible");logged=true;}
    return;
   }
   ancestor=*reinterpret_cast<std::uint8_t**>(ancestor+0x10);
  }
 }
 originalRemoveTag(entity,tag);
}
thread_local const glm::mat4* weaponHand=nullptr;
void AimHook(void* player,glm::vec3* origin,glm::vec3* direction){
 if(weaponHand&&player==LocalPlayer()){*origin=glm::vec3((*weaponHand)[3]);*direction=Controllers::Aim(*weaponHand);return;}
 originalAim(player,origin,direction);
 Snapshot s;glm::mat4 left,right;if(!Copy(s)||!WorldHands(player,s,left,right))return;
 // Body heading remains headset-driven. Tool/grab rays come from the hand
 // performing the action; the engine still supports one native grab at a time.
 auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress())-reinterpret_cast<std::uintptr_t>(gameBase);
 bool firing=CurrentBuild::WeaponAimCaller(caller);
 auto hand=(!firing&&s.valid[0]&&(!s.valid[1]||s.buttons.interact||(s.buttons.leftGrab&&!s.buttons.grab)))?left:right;
 if(firing&&!s.valid[1])return;
 *origin=glm::vec3(hand[3]);*direction=Controllers::Aim(firing?SupportGrip::Aim(hand):hand);
 static bool logged=false;if(!logged){Log("Controller interaction rays active; head drives body heading");logged=true;}
}
Transform* CameraHook(void* player){
 if(weaponHand&&player==LocalPlayer()){thread_local Transform hand;hand=Controllers::Pack(*weaponHand);return &hand;}
 auto result=originalCamera(player);
 auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress())-reinterpret_cast<std::uintptr_t>(gameBase);
 if(!CurrentBuild::WeaponCameraCaller(caller))return result;
 Snapshot s;glm::mat4 left,right;
 if(!Copy(s)||!s.valid[1]||!WorldHands(player,s,left,right))return result;
 thread_local Transform aim;aim=Controllers::Pack(SupportGrip::Aim(right));
 static bool logged=false;if(!logged){Log("Native weapon camera aim redirected to tracked right hand");logged=true;}
 return &aim;
}
// Native fire/spray routines also read the camera fields directly. Substitute
// only during those routines, then restore before movement or scene rendering.
struct WeaponAimScope {
 std::uint8_t* player=nullptr;Transform saved{};glm::mat4 hand{1.f};const glm::mat4* previous=nullptr;
 WeaponAimScope(void* tool,bool spray=false){
  if(!Readable(tool,16))return;
  auto p=*reinterpret_cast<std::uint8_t**>(static_cast<std::uint8_t*>(tool)+8);
  Snapshot sample;glm::mat4 left,right;
  if(!Copy(sample)||!sample.valid[1]||!WorldHands(p,sample,left,right))return;
  right=SupportGrip::Aim(right);player=p;hand=right;std::memcpy(&saved,p+0x640,sizeof(saved));
  auto camera=right;
  if(spray)camera[3]-=glm::vec4(glm::mat3(right)*glm::vec3(.3f,-.2f,-.7f),0.f);
  auto pose=Controllers::Pack(camera);std::memcpy(p+0x640,&pose,sizeof(pose));previous=weaponHand;weaponHand=&hand;
 }
 ~WeaponAimScope(){if(player){std::memcpy(player+0x640,&saved,sizeof(saved));weaponHand=previous;}}
};
using FireFn=void(*)(void*);
FireFn fireOriginal[3]{};ToolFn sprayOriginal=nullptr;
void Fire0(void* tool){WeaponAimScope aim(tool);fireOriginal[0](tool);}
void Fire1(void* tool){WeaponAimScope aim(tool);fireOriginal[1](tool);}
void Fire2(void* tool){WeaponAimScope aim(tool);fireOriginal[2](tool);}
void Spray(void* tool,float dt){WeaponAimScope aim(tool,true);sprayOriginal(tool,dt);}
LONG AttachWeaponAim(){
 const unsigned sites[]={0x44eaf0,0x44fef0,0x450250};FireFn hooks[]={Fire0,Fire1,Fire2};
 for(int i=0;i<3;i++){fireOriginal[i]=reinterpret_cast<FireFn>(gameBase+sites[i]);auto result=DetourAttach(reinterpret_cast<PVOID*>(&fireOriginal[i]),reinterpret_cast<PVOID>(hooks[i]));if(result!=NO_ERROR)return result;}
 sprayOriginal=reinterpret_cast<ToolFn>(gameBase+0x463cd0);return DetourAttach(reinterpret_cast<PVOID*>(&sprayOriginal),reinterpret_cast<PVOID>(&Spray));
}
namespace NativeContact { bool Closest(void*,glm::vec3,float,glm::vec3&,glm::vec3&); }
#include "dual_grab.h"
#include "native_contact.h"
#include "world_contact.h"
#include "avatar_pose.h"
#include "hands_only.h"
#include "vr_hands_tool.h"
#include "vehicle_camera.h"
#include "player_assists.h"
#include "accessories.h"
void UpdateInteraction(void* player){
 auto p=static_cast<std::uint8_t*>(player);if(!Readable(p,0x9A0)){interactionTarget=0;return;}
 auto vehicle=*reinterpret_cast<void**>(p+0x998);
 if(vehicle){interactionTarget=(reinterpret_cast<std::uintptr_t>(vehicle)&~3ull)|1;return;}
 auto shape=*reinterpret_cast<std::uint8_t**>(p+0x970);
 if(!Readable(shape,0x18)){interactionTarget=0;return;}
 static const TagName interact("interact"),avf("AVF_Parent"),weaponHost("weapon_host");
 auto usable=[&](void* e){return hasTag(e,&interact)||hasTag(e,&avf)||hasTag(e,&weaponHost);};
 std::uint8_t* ordinary=usable(shape)?shape:nullptr;
 auto parent=*reinterpret_cast<std::uint8_t**>(shape+0x10);
 for(int n=0;n<32&&Readable(parent,0x18);n++,parent=*reinterpret_cast<std::uint8_t**>(parent+0x10)){
  if(!ordinary&&*reinterpret_cast<unsigned short*>(parent+8)==7){interactionTarget=(reinterpret_cast<std::uintptr_t>(parent)&~3ull)|2;return;}
  if(!ordinary&&usable(parent))ordinary=parent;
 }
 interactionTarget=ordinary?(reinterpret_cast<std::uintptr_t>(ordinary)&~3ull)|1:0;
}
void PlayerHook(void* player,float dt){
 Snapshot s;bool active=Copy(s)&&player==LocalPlayer()&&Readable(player,0xAB0);
 auto p=static_cast<std::uint8_t*>(player);
 if(player==LocalPlayer()){auto vehicle=active?*reinterpret_cast<void**>(p+0x998):nullptr;static void* lastVehicle=nullptr;if(vehicle!=lastVehicle){vehicleHeight=vehicle?Controllers::SeatHeight(s.head[3].y):0.f;vehicleAdjustAt=0;lastVehicle=vehicle;}inVehicle=vehicle!=nullptr;}
 if(active&&headFollow&&!inVehicle&&!HUDMath::interactive){
  Transform camera;std::memcpy(&camera,p+0x640,sizeof(camera));
  if(Controllers::Valid(camera)){
   if(!headingReady||headingPlayer!=player||headingCenter!=s.center){
    auto m=Controllers::Matrix(camera);referenceYaw=std::atan2(m[2].x,m[2].z);nativeReferenceYaw=*reinterpret_cast<float*>(p+0x704);
    turnYaw=0;headingPlayer=player;headingCenter=s.center;headingReady=true;
   }
   float yaw=std::atan2(s.head[2].x,s.head[2].z);
   float pitch=std::asin((std::clamp)(-s.head[2].y,-1.f,1.f));
   *reinterpret_cast<float*>(p+0x704)=nativeReferenceYaw+turnYaw.load()+yaw;
   *reinterpret_cast<float*>(p+0x700)=pitch;
  }
 }else if(player==headingPlayer)headingReady=false;
 PlayerAssists::AmmoScope ammo(active?player:nullptr);
 float health=active?*reinterpret_cast<float*>(p+0x7C8):0;
 Phase("native player update");auto updateStart=GetTickCount64();originalPlayer(player,dt);
 if(active){ammo.Restore(player);if(PlayerAssists::godMode&&std::isfinite(health)&&health>0)*reinterpret_cast<float*>(p+0x7C8)=(std::max)(health,*reinterpret_cast<float*>(p+0x7C8));}
 if(GetTickCount64()-updateStart>250)Log("Slow native player update ms="+std::to_string(GetTickCount64()-updateStart));
 Phase("VR hands registration/selection");if(active)VRHandsTool::Tick(player);
 if(active)VehicleCamera::Tick(player,s);
 Phase("VR contacts and grabbing");
 if(player==LocalPlayer()){DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);UpdateInteraction(player);static void* lastNativeGrab=nullptr;auto grabbed=*reinterpret_cast<void**>(p+0x788);if(active&&grabbed&&grabbed!=lastNativeGrab)Haptics::Grab(s.buttons.leftGrab&&!s.buttons.grab?0:1);lastNativeGrab=active?grabbed:nullptr;auto contactStart=GetTickCount64();HandsOnlyVisual::Update(player);WorldContact::Tick(player,s,active&&!HUDMath::interactive&&foreground==GetCurrentProcessId());DualGrab::Tick(player,s,active&&!HUDMath::interactive&&foreground==GetCurrentProcessId());if(GetTickCount64()-contactStart>100)Log("Slow VR contacts/grabs ms="+std::to_string(GetTickCount64()-contactStart));}
 // Keep the native flashlight transform: it includes the light's orientation
 // correction and offset. Player yaw/pitch above already follows the headset.
 Phase("game update after player hook");
}
void VisibilityHook(void* player,bool visible){
 Snapshot s;bool previous=showingLocalBody;
 if(Copy(s)&&player==LocalPlayer()&&Readable(player,0x2DCC)){
  // The native screenshot-character path removes "invisible" on the local
  // character rig; it does not switch the camera or teleport the player.
  visible=true;showingLocalBody=true;
  static bool logged=false;if(!logged){Log("VR local-character visibility enabled");logged=true;}
 }
 originalVisibility(player,visible);showingLocalBody=previous;
}
void Sample(const std::array<vr::TrackedDevicePose_t,vr::k_unMaxTrackedDeviceCount>& poses,const glm::mat4& center,unsigned serial){
 Snapshot s;s.stamp=GetTickCount64();s.center=serial;s.head=center*Stereo::Pose(poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);vr::VRControllerState_t state[2]{};vr::TrackedDeviceIndex_t devices[2]{};
 for(int hand=0;hand<2;hand++){
  auto role=hand?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand;
  auto index=systemVR->GetTrackedDeviceIndexForControllerRole(role);devices[hand]=index;
  s.valid[hand]=index<poses.size()&&poses[index].bDeviceIsConnected&&poses[index].bPoseIsValid&&systemVR->GetControllerState(index,&state[hand],sizeof(state[hand]));
  if(s.valid[hand])s.hand[hand]=center*Stereo::Pose(poses[index].mDeviceToAbsoluteTracking);
 }
 s.input=(s.valid[0]||s.valid[1])&&systemVR->IsInputAvailable();if(s.input)s.buttons=Controllers::Map(state[0],state[1]);
 Haptics::Flush(devices,s.valid,s.input);
 static int last=-1;int mask=int(s.valid[0])+2*int(s.valid[1]);if(mask!=last){Log("Controllers tracked: left="+std::to_string(s.valid[0])+" right="+std::to_string(s.valid[1]));last=mask;}
 SnapshotCache::Publish(mutex,latest,s);
}
DWORD WINAPI InputThread(void*){
 bool held[12]{};const WORD keys[10]={'W','S','A','D',VK_SPACE,VK_CONTROL,'E','F',VK_SHIFT,'R'};
 ULONGLONG meleeUntil=0;
 Controllers::XButtonGesture xGesture;ULONGLONG reloadUntil=0,interactUntil=0;
 ULONGLONG last=GetTickCount64(),mapKeyUntil=0,pauseKeyUntil=0;Controllers::WheelRepeat wheelRepeat;float mouseRemainder=0;
 auto mapKey=[](bool down){INPUT key{};key.type=INPUT_KEYBOARD;key.ki.wScan=WORD(MapVirtualKeyW(VK_TAB,MAPVK_VK_TO_VSC));key.ki.dwFlags=KEYEVENTF_SCANCODE|(down?0:KEYEVENTF_KEYUP);SendInput(1,&key,sizeof(key));};
 for(;;){
  Sleep(10);auto now=GetTickCount64();float dt=float((std::min)(now-last,ULONGLONG(50)))/1000.f;last=now;
  Snapshot s;DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
  if(mapKeyUntil&&(now>=mapKeyUntil||pid!=GetCurrentProcessId()||!enabled)){mapKey(false);mapKeyUntil=0;}
  if(mapRequested.exchange(false)&&enabled&&pid==GetCurrentProcessId()){mapKey(true);mapKeyUntil=now+80;}
  auto pauseKey=[](bool down){INPUT key{};key.type=INPUT_KEYBOARD;key.ki.wScan=WORD(MapVirtualKeyW(VK_ESCAPE,MAPVK_VK_TO_VSC));key.ki.dwFlags=KEYEVENTF_SCANCODE|(down?0:KEYEVENTF_KEYUP);SendInput(1,&key,sizeof(key));};
  if(pauseKeyUntil&&(now>=pauseKeyUntil||pid!=GetCurrentProcessId()||!enabled)){pauseKey(false);pauseKeyUntil=0;}
  if(pauseRequested.exchange(false)&&enabled&&pid==GetCurrentProcessId()){pauseKey(true);pauseKeyUntil=now+80;}
  bool active=Copy(s)&&s.input&&pid==GetCurrentProcessId()&&!menuOpen&&!HUDMath::interactive;auto b=active?s.buttons:Controllers::Buttons{};if(Accessories::Locked())b.wheel=0;if(Accessories::Occupied()){b.leftTrigger=false;b.fire=false;}
  auto target=interactionTarget.load();
  auto x=xGesture.Update(b.interact,active,now,int(target&3),target&~3ull);
  if(!active){reloadUntil=interactUntil=0;}
  if(x.reload)reloadUntil=now+80;if(x.interact)interactUntil=now+80;
  bool driving=active&&inVehicle.load();
  if(driving)b=Controllers::VehicleButtons(b);
  bool swing=Accessories::ConsumeMelee();
  if(!active||driving||!physicalMelee||Accessories::Occupied()||Accessories::Locked()||heldObjects.load()||(meleeUntil&&Accessories::meleeTool.load()!=Accessories::SelectedTool()))meleeUntil=0;
  else if(swing)meleeUntil=now+100;
  b.fire=b.fire||(now<meleeUntil);
  bool secondary=Controllers::SecondaryAction(b,driving,handsOnly,separateGrab,DualGrab::triggerBlockMask.load());
  bool desired[12]={b.forward,b.back,b.left,b.right,b.jump,b.crouch,now<interactUntil,b.light,b.sprint,now<reloadUntil,b.fire&&(driving||(!handsOnly&&(!separateGrab||!(DualGrab::triggerBlockMask.load()&2)))),secondary};
  for(int i=0;i<12;i++)if(held[i]!=desired[i]){
   INPUT in{};if(i<10){in.type=INPUT_KEYBOARD;in.ki.wScan=WORD(MapVirtualKeyW(keys[i],MAPVK_VK_TO_VSC));in.ki.dwFlags=KEYEVENTF_SCANCODE|(desired[i]?0:KEYEVENTF_KEYUP);}
   else{in.type=INPUT_MOUSE;in.mi.dwFlags=i==10?(desired[i]?MOUSEEVENTF_LEFTDOWN:MOUSEEVENTF_LEFTUP):(desired[i]?MOUSEEVENTF_RIGHTDOWN:MOUSEEVENTF_RIGHTUP);}
   if(SendInput(1,&in,sizeof(in))==1)held[i]=desired[i];
  }
  if(active&&!driving&&b.turn!=0&&headFollow&&headingReady){turnYaw=turnYaw.load()-b.turn*glm::radians(turnSpeed.load())*dt;}
  else if(active&&b.turn!=0){mouseRemainder+=b.turn*(driving?vehicleTurnSpeed.load()*5.f:450.f)*dt;LONG dx=LONG(mouseRemainder);mouseRemainder-=float(dx);if(dx){INPUT in{};in.type=INPUT_MOUSE;in.mi.dx=dx;in.mi.dwFlags=MOUSEEVENTF_MOVE;SendInput(1,&in,sizeof(in));}}
  else mouseRemainder=0;
  if(wheelRepeat.Update(b.wheel,true,now)){INPUT in{};in.type=INPUT_MOUSE;in.mi.dwFlags=MOUSEEVENTF_WHEEL;in.mi.mouseData=DWORD(b.wheel*WHEEL_DELTA);SendInput(1,&in,sizeof(in));}
 }
}
void Prepare(std::uint8_t* base){gameBase=base;AvatarPose::originalVisual=reinterpret_cast<AvatarPose::VisualFn>(base+0x32c10);AvatarPose::original=reinterpret_cast<AvatarPose::InterpolateFn>(base+0x2f4e0);Accessories::argumentReader=reinterpret_cast<Accessories::ReadFn>(base+0x640c30);Accessories::resultWriter=reinterpret_cast<Accessories::WriteFn>(base+0x63f850);Accessories::originalHealth=reinterpret_cast<Accessories::LuaFn>(base+0x4855c0);PlayerAssists::originalDamage=reinterpret_cast<PlayerAssists::DamageFn>(base+0xc7b60);originalGetHands=reinterpret_cast<GetHandsFn>(base+0xc0c00);hasTag=reinterpret_cast<HasTagFn>(base+0x15baa0);originalRemoveTag=reinterpret_cast<RemoveTagFn>(base+0x15bba0);setTag=reinterpret_cast<SetTagFn>(base+0x15bd80);originalCamera=reinterpret_cast<CameraFn>(base+CurrentBuild::PlayerCameraRva);originalVisibility=reinterpret_cast<VisibilityFn>(base+CurrentBuild::BodyVisibilityRva);originalPlayer=reinterpret_cast<ToolFn>(base+CurrentBuild::PlayerUpdateRva);originalTool=reinterpret_cast<ToolFn>(base+CurrentBuild::ToolUpdateRva);originalAim=reinterpret_cast<AimFn>(base+CurrentBuild::AimRva);setBody=reinterpret_cast<BodyFn>(base+CurrentBuild::BodyTransformRva);setHands=reinterpret_cast<HandsFn>(base+CurrentBuild::HandPosesRva);}
LONG Attach(){auto error=DetourAttach(reinterpret_cast<PVOID*>(&AvatarPose::original),reinterpret_cast<PVOID>(&AvatarPose::Hook));if(error!=NO_ERROR)return error;error=DetourAttach(reinterpret_cast<PVOID*>(&AvatarPose::originalVisual),reinterpret_cast<PVOID>(&AvatarPose::VisualHook));if(error!=NO_ERROR)return error;error=DetourAttach(reinterpret_cast<PVOID*>(&originalTool),reinterpret_cast<PVOID>(&ToolHook));if(error==NO_ERROR)error=DetourAttach(reinterpret_cast<PVOID*>(&originalAim),reinterpret_cast<PVOID>(&AimHook));if(error==NO_ERROR)error=DetourAttach(reinterpret_cast<PVOID*>(&setBody),reinterpret_cast<PVOID>(&BodyHook));if(error==NO_ERROR)error=DetourAttach(reinterpret_cast<PVOID*>(&originalPlayer),reinterpret_cast<PVOID>(&PlayerHook));if(error==NO_ERROR)error=DetourAttach(reinterpret_cast<PVOID*>(&originalVisibility),reinterpret_cast<PVOID>(&VisibilityHook));if(error==NO_ERROR)error=DetourAttach(reinterpret_cast<PVOID*>(&originalCamera),reinterpret_cast<PVOID>(&CameraHook));if(error==NO_ERROR)error=DetourAttach(reinterpret_cast<PVOID*>(&originalGetHands),reinterpret_cast<PVOID>(&GetHandsHook));if(error==NO_ERROR)error=DetourAttach(reinterpret_cast<PVOID*>(&originalRemoveTag),reinterpret_cast<PVOID>(&RemoveTagHook));if(error==NO_ERROR)error=DetourAttach(reinterpret_cast<PVOID*>(&PlayerAssists::originalDamage),reinterpret_cast<PVOID>(&PlayerAssists::Damage));if(error==NO_ERROR)error=DetourAttach(reinterpret_cast<PVOID*>(&Accessories::originalHealth),reinterpret_cast<PVOID>(&Accessories::Health));if(error==NO_ERROR)error=AttachWeaponAim();return error;}
}
