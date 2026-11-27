#pragma once
// Included in ControllerBridge. All physics writes run on the player-update
// thread, not the input or render workers. Handles are revalidated every tick.
namespace DualGrab {
std::atomic<unsigned> fallbackMask{0}, triggerBlockMask{0};
struct Bounds {glm::vec3 low,high;};static_assert(sizeof(Bounds)==24);
using BoundsFn=void*(*)(void*,Bounds*);using WakeFn=void(*)(void*,bool);
BoundsFn boundsFunction=nullptr;WakeFn wakeFunction=nullptr;
struct Grip {std::uint8_t* body=nullptr;int id=0,searchCursor=0;bool down=false,physical=false,throwConsumed=false;GrabMath::TriggerThrow throwing;glm::vec3 anchor{},last{},velocity{},handOffset{};glm::mat4 localRotation{1.f};ULONGLONG stamp=0,nextSearch=0;} grip[2];
struct Entities {
 void** positive=nullptr;void** negative=nullptr;int positiveCount=0,negativeCount=0;
 void* Get(int id)const {if(id>0&&id<positiveCount)return positive[id];if(id<0&&id>-negativeCount)return negative[-id];return nullptr;}
 int Count()const{return (std::max)(0,positiveCount-1)+(std::max)(0,negativeCount-1);}
 int Handle(int slot)const{int positives=(std::max)(0,positiveCount-1);return slot<positives?slot+1:-(slot-positives+1);}
};
bool Scene(Entities& entities){
 auto game=*reinterpret_cast<std::uint8_t**>(gameBase+0x1CA7E70);if(!Readable(game,0x58))return false;
 auto scene=*reinterpret_cast<std::uint8_t**>(game+0x50);if(!Readable(scene,0xA28))return false;
 entities.positiveCount=*reinterpret_cast<int*>(scene+0xA18);entities.positive=*reinterpret_cast<void***>(scene+0xA20);
 entities.negativeCount=*reinterpret_cast<int*>(scene+0x208);entities.negative=*reinterpret_cast<void***>(scene+0x210);
 auto valid=[](void** table,int count){return count>=0&&count<500000&&(!count||Readable(table,std::size_t(count)*sizeof(void*)));};
 return valid(entities.positive,entities.positiveCount)&&valid(entities.negative,entities.negativeCount);
}
bool Valid(const Grip& g,const Entities& entities){return g.body&&entities.Get(g.id)==g.body&&Readable(g.body,0x148)&&*reinterpret_cast<unsigned short*>(g.body+8)==1&&g.body[0x138];}
void NearbyCandidates(const Entities&,glm::vec3,void*,void*,Grip&,float&,glm::vec3&);
void AimCandidate(const Entities&,const glm::mat4&,void*,Grip&,glm::vec3&);
void Candidate(const Entities& entities,int id,glm::vec3 pos,void* tool,void* other,Grip& result,float& nearest,glm::vec3& contact){
 auto body=static_cast<std::uint8_t*>(entities.Get(id));static const TagName bone("bone"),accessory("sledge_accessory");
 if(!body||body==tool||!Readable(body,0x148)||*reinterpret_cast<unsigned short*>(body+8)!=1||!body[0x138]||hasTag(body,&bone)||hasTag(body,&accessory))return;
 // Individual shapes prevent selecting the empty space between body parts.
 auto child=*reinterpret_cast<std::uint8_t**>(body+0x28);
 auto deadline=GetTickCount64()+1;
 for(int n=0;n<32&&Readable(child,0xBC)&&GetTickCount64()<=deadline;n++,child=*reinterpret_cast<std::uint8_t**>(child+0x18)){
  if(*reinterpret_cast<unsigned short*>(child+8)!=2)continue;
  Bounds box;std::memcpy(&box,child+0xA4,sizeof(box));bool sane=true;
  for(int a=0;a<3;a++)if(!std::isfinite(box.low[a])||!std::isfinite(box.high[a])||box.low[a]>box.high[a])sane=false;
  if(!sane)continue;auto point=glm::clamp(pos,box.low,box.high);float distance=glm::length(point-pos);
  if(distance<nearest){glm::vec3 surface,normal;
   if(NativeContact::Closest(child,pos,nearest,surface,normal)){distance=glm::length(surface-pos);if(distance<nearest){nearest=distance;result.body=body;result.id=id;contact=surface;}}
  }
 }
}
void Velocity(std::uint8_t* body,const glm::vec3& linear,const glm::vec3& angular){
 std::memcpy(body+0xD8,&linear,12);std::memcpy(body+0xE4,&angular,12);
 (wakeFunction?wakeFunction:reinterpret_cast<WakeFn>(gameBase+0x31CC0))(body,false);
}
void Tick(void* player,const Snapshot& sample,bool active){
 Entities entities;
 if(!Scene(entities)){fallbackMask=0;triggerBlockMask=0;heldObjects=0;for(auto& g:grip)g={};return;}
 glm::mat4 hands[2];if(!active||!sample.input||menuOpen||!separateGrab||inVehicle||!WorldHands(player,sample,hands[0],hands[1])){fallbackMask=0;triggerBlockMask=0;heldObjects=0;for(auto& g:grip){if(Valid(g,entities))Velocity(g.body,glm::vec3(0),glm::vec3(0));g={};}return;}
 unsigned fallback=0,blocked=0;
 bool trigger[2]={sample.buttons.leftTrigger,sample.buttons.fire};
 bool down[2]={sample.buttons.leftGrab,sample.buttons.grab};
 auto tool=*reinterpret_cast<void**>(static_cast<std::uint8_t*>(player)+0x820);
 for(int hand=0;hand<2;hand++){
  auto& g=grip[hand];if((Accessories::Occupied()&(1u<<hand))||(hand==0&&SupportGrip::active)||!sample.valid[hand]){if(Valid(g,entities)&&g.body!=grip[1-hand].body)Velocity(g.body,glm::vec3(0),glm::vec3(0));g={};continue;}glm::vec3 pos=glm::vec3(hands[hand]*glm::vec4(g.body?g.handOffset:glm::vec3(0),1));
  if(sample.stamp!=g.stamp){if(g.stamp&&sample.stamp>g.stamp&&sample.stamp-g.stamp<150)g.velocity=GrabMath::Limit((pos-g.last)/(float(sample.stamp-g.stamp)/1000.f),12.f);else g.velocity=glm::vec3(0);g.last=pos;g.stamp=sample.stamp;}
  if(g.body&&!Valid(g,entities)){g.body=nullptr;g.id=0;}
  if(!trigger[hand])g.throwConsumed=false;
  if(g.body&&trigger[hand])g.throwConsumed=true;
  float launch=g.throwing.Update(trigger[hand],g.body!=nullptr,sample.stamp);
  if(launch>0){if(g.body!=grip[1-hand].body)Velocity(g.body,GrabMath::Limit(g.velocity+Controllers::Aim(hands[hand])*launch,15.f),glm::vec3(0));g.body=nullptr;g.id=0;}
  if(!down[hand])g.physical=false;
  if(!down[hand]&&g.body){if(g.body!=grip[1-hand].body)Velocity(g.body,g.velocity,glm::vec3(0));g.body=nullptr;g.id=0;}
  if(down[hand]&&!g.physical&&!g.body&&(!g.down||sample.stamp>=g.nextSearch)){
   g.nextSearch=sample.stamp+150;
   float nearest=.18f;glm::vec3 contact;static const TagName bone("bone"),accessory("sledge_accessory");
   auto bounds=boundsFunction?boundsFunction:reinterpret_cast<BoundsFn>(gameBase+0x2F2B0);
   LARGE_INTEGER start,freq;QueryPerformanceCounter(&start);QueryPerformanceFrequency(&freq);
   int total=entities.Count();
   NearbyCandidates(entities,pos,tool,nullptr,g,nearest,contact);
   if(grip[1-hand].body)Candidate(entities,grip[1-hand].id,pos,tool,nullptr,g,nearest,contact);
   auto target=*reinterpret_cast<std::uint8_t**>(static_cast<std::uint8_t*>(player)+0x970);
   for(int parent=0;parent<8&&Readable(target,0x30);parent++,target=*reinterpret_cast<std::uint8_t**>(target+0x10)){
    if(*reinterpret_cast<unsigned short*>(target+8)==1){Candidate(entities,*reinterpret_cast<int*>(target+0xC),pos,tool,grip[1-hand].body,g,nearest,contact);break;}
   }
   for(int searched=0;!g.body&&searched<(std::min)(total,128);searched++){
    LARGE_INTEGER now;QueryPerformanceCounter(&now);if(double(now.QuadPart-start.QuadPart)/double(freq.QuadPart)>.002)break;
    if(g.searchCursor>=total)g.searchCursor=0;
    int id=entities.Handle(g.searchCursor++);auto body=static_cast<std::uint8_t*>(entities.Get(id));if(!body||body==tool||!Readable(body,0x148)||*reinterpret_cast<unsigned short*>(body+8)!=1||!body[0x138]||hasTag(body,&bone)||hasTag(body,&accessory))continue;
    Bounds box{};bounds(body,&box);bool sane=true;for(int axis=0;axis<3;axis++)if(!std::isfinite(box.low[axis])||!std::isfinite(box.high[axis])||box.high[axis]<box.low[axis])sane=false;
    if(!sane)continue;
    auto point=glm::clamp(pos,box.low,box.high);float distance=glm::length(point-pos);
    if(distance<nearest)Candidate(entities,id,pos,tool,grip[1-hand].body,g,nearest,contact);
   }
   g.handOffset=glm::vec3(0);
   if(!g.body){AimCandidate(entities,hands[hand],tool,g,contact);if(g.body){g.handOffset=glm::vec3(glm::inverse(hands[hand])*glm::vec4(contact,1));g.last=contact;g.velocity=glm::vec3(0);}}
   if(!g.body)g.nextSearch=sample.stamp+16;
   if(g.body){Transform t;std::memcpy(&t,g.body+0x40,sizeof(t));if(Controllers::Valid(t)){g.physical=true;Haptics::Grab(hand);auto body=Controllers::Matrix(t);g.anchor=glm::vec3(glm::inverse(body)*glm::vec4(contact,1));g.localRotation=glm::inverse(hands[hand])*body;Log(std::string("Independent grab ")+(hand?"right":"left")+" body="+std::to_string(g.id));}else{g.body=nullptr;g.id=0;}}
  }

  if(down[hand]&&!g.physical)fallback|=1u<<hand;
  if(g.physical||g.throwConsumed)blocked|=1u<<hand;
  g.down=down[hand];
 }
 // Solve each body once, after both releases/acquisitions. Never let the
 // second controller overwrite the first controller's body velocity.
 for(int h=0;h<2;h++){
  auto& g=grip[h];if(!g.body||(h==1&&g.body==grip[0].body))continue;
  Transform t;std::memcpy(&t,g.body+0x40,sizeof(t));if(!Controllers::Valid(t)){g.body=nullptr;g.id=0;continue;}
  auto body=Controllers::Matrix(t);auto anchor=glm::vec3(body*glm::vec4(g.anchor,1));glm::vec3 target=glm::vec3(hands[h]*glm::vec4(g.handOffset,1));
  if(glm::length(anchor-target)>1.5f){g.body=nullptr;g.id=0;continue;}
  auto rotation=hands[h]*g.localRotation;auto velocity=g.velocity;
  if(h==0&&g.body==grip[1].body){
   auto& other=grip[1];auto midpoint=(g.anchor+other.anchor)*.5f;
   auto targetOther=glm::vec3(hands[1]*glm::vec4(other.handOffset,1));
   rotation=ContactMath::TwoHand(rotation,g.anchor,other.anchor,target,targetOther);
   anchor=glm::vec3(body*glm::vec4(midpoint,1));target=(target+targetOther)*.5f;velocity=(g.velocity+other.velocity)*.5f;
  }
  Velocity(g.body,GrabMath::FollowVelocity(anchor,target,velocity),GrabMath::AngularVelocity(body,rotation));
 }
 heldObjects=(grip[0].body?1u:0u)|(grip[1].body?2u:0u);triggerBlockMask=blocked;fallbackMask=fallback;
}
}
