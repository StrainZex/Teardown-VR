#pragma once
// Native surface rays constrain contacts; boxes are only grab broad-phase/probe
// seeds. All physics writes remain on the game update thread.
namespace WorldContact {
constexpr float HandRadius=.018f,ToolRadius=.002f;
struct Entry {int id=0;std::uint8_t* body=nullptr;ULONGLONG seen=0,lastPush=0;DualGrab::Bounds bounds{};std::array<DualGrab::Bounds,32> shapes{};int shapeCount=0;Transform pose{};};
std::array<Entry,128> nearby{};int cursor=0,refreshCursor=0;void* lastScene=nullptr;
glm::vec3 previous[2]{},previousWanted[2]{};bool initialized[2]{};ULONGLONG previousStamp=0;
DualGrab::Entities current;
bool Valid(const Entry& e){return e.body&&current.Get(e.id)==e.body&&Readable(e.body,0x148)&&*reinterpret_cast<unsigned short*>(e.body+8)==1;}
void RefreshShapes(Entry& entry){
 entry.shapeCount=0;std::memcpy(&entry.pose,entry.body+0x40,sizeof(entry.pose));auto child=*reinterpret_cast<std::uint8_t**>(entry.body+0x28);
 // Native body bounds (2E990) unions these shape bounds. Use each shape
 // independently so gaps between a body's separate shapes stay empty.
 for(int visited=0;visited<256&&entry.shapeCount<32&&Readable(child,0xBC);visited++,child=*reinterpret_cast<std::uint8_t**>(child+0x18)){
  if(*reinterpret_cast<unsigned short*>(child+8)!=2)continue;
  DualGrab::Bounds box;std::memcpy(&box,child+0xA4,sizeof(box));bool valid=true;
  for(int axis=0;axis<3;axis++)if(!std::isfinite(box.low[axis])||!std::isfinite(box.high[axis])||box.low[axis]>box.high[axis])valid=false;
  if(valid)entry.shapes[entry.shapeCount++]=box;
 }
 if(entry.shapeCount){entry.bounds=entry.shapes[0];for(int i=1;i<entry.shapeCount;i++){entry.bounds.low=glm::min(entry.bounds.low,entry.shapes[i].low);entry.bounds.high=glm::max(entry.bounds.high,entry.shapes[i].high);}}
}
bool ShapeSweep(const Entry& entry,glm::vec3 from,glm::vec3 to,float radius,float& at,glm::vec3& normal){
 bool hit=false;at=1.f;
 for(int i=0;i<entry.shapeCount;i++){float time;glm::vec3 n;auto& box=entry.shapes[i];
  if(ContactMath::Sweep(from,to,box.low,box.high,radius,time,n)&&time<at){at=time;normal=n;hit=true;}
 }
 return hit;
}
struct PushStamp {void* body=nullptr;ULONGLONG time=0;};
std::array<PushStamp,16> pushed{};
using ImpulseFn=void(*)(void*,const glm::vec3*,const glm::vec3*,float,bool);
ImpulseFn impulseFunction=nullptr;
glm::vec3 Move(glm::vec3 from,glm::vec3 to,float radius,float dt,void* ignore,bool push,const glm::vec3* handMotion=nullptr){
 auto delta=to-from;float length=glm::length(delta);if(length<.00001f||length>1.f)return to;
 float distance=length;glm::vec3 normal(0);std::uint8_t* body=nullptr;
 bool hit=false;glm::vec3 hitOffset(0);
 const glm::vec3 offsets[]={glm::vec3(0),{radius,0,0},{-radius,0,0},{0,radius,0},{0,-radius,0},{0,0,radius},{0,0,-radius}};
 for(auto offset:offsets){float d=length;glm::vec3 n;std::uint8_t* b=nullptr;
  if(NativeContact::Ray(from+offset,to+offset,ignore,d,n,b)&&d<=distance){distance=d;normal=n;body=b;hitOffset=offset;hit=true;}
 }
 if(!hit)return to;
 auto now=GetTickCount64();PushStamp* slot=&pushed[0];
 for(auto& entry:pushed){if(entry.body==body){slot=&entry;break;}if(entry.time<slot->time)slot=&entry;}
 if(push&&body[0x138]&&dt>.001f&&(slot->body!=body||now-slot->time>=50)){
  *slot={body,now};glm::vec3 velocity,angular;std::memcpy(&velocity,body+0xD8,12);std::memcpy(&angular,body+0xE4,12);
  float inverseMass=0;std::memcpy(&inverseMass,body+0xFC,4);
  auto impulse=ContactMath::PushVelocity(handMotion?*handMotion:delta/dt,velocity,normal,inverseMass);
  if(inverseMass>0&&std::isfinite(inverseMass)&&glm::length(impulse)>.00001f){
   auto force=GrabMath::Limit(impulse/inverseMass,2.f);auto point=from+hitOffset+delta*(distance/length);
   // Apply at the contact point so native inertia and door joints can respond.
   (impulseFunction?impulseFunction:reinterpret_cast<ImpulseFn>(gameBase+0x2deb0))(body,&point,&force,3.402823466e+38F,true);
  }
 }
 auto contact=from+delta*((std::max)(0.f,distance-.001f)/length);
 auto rest=to-contact;auto slide=rest-normal*(std::min)(0.f,glm::dot(rest,normal));
 float slideDistance=glm::length(slide);glm::vec3 slideNormal;std::uint8_t* slideBody=nullptr;
 if(slideDistance>.0001f&&NativeContact::Ray(contact+hitOffset,contact+hitOffset+slide,ignore,slideDistance,slideNormal,slideBody))slide=glm::normalize(slide)*(std::max)(0.f,slideDistance-.001f);
 return contact+slide;
}
void Tick(void* player,const Snapshot& sample,bool active){
 if(!active||!physicalContact||menuOpen||inVehicle||!sample.input||!DualGrab::Scene(current)){
  initialized[0]=initialized[1]=false;previousStamp=0;nearby={};cursor=0;
  std::lock_guard<std::mutex> guard(contactMutex);contactStamp=0;return;
 }
 // Several game updates may consume one tracking sample. Do not reset the
 // sweep origin or derive zero-dt velocity on those duplicate updates.
 if(sample.stamp==previousStamp)return;
 auto scene=*reinterpret_cast<void**>(*reinterpret_cast<std::uint8_t**>(gameBase+0x1c96e70)+0x50);
 if(scene!=lastScene){nearby={};cursor=0;initialized[0]=initialized[1]=false;lastScene=scene;}
 glm::mat4 left,right;{std::lock_guard<std::mutex> guard(contactMutex);contactStamp=0;}
 if(!WorldHands(player,sample,left,right,false))return;
 glm::vec3 wanted[2]={glm::vec3(left[3]),glm::vec3(right[3])};
 auto tool=*reinterpret_cast<void**>(static_cast<std::uint8_t*>(player)+0x820);
 auto bounds=DualGrab::boundsFunction?DualGrab::boundsFunction:reinterpret_cast<DualGrab::BoundsFn>(gameBase+0x2f2a0);static const TagName bone("bone"),accessory("sledge_accessory");
 int total=current.Count();auto now=GetTickCount64();
 LARGE_INTEGER frequency,start;QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&start);
 auto overBudget=[&](){LARGE_INTEGER t;QueryPerformanceCounter(&t);return double(t.QuadPart-start.QuadPart)/double(frequency.QuadPart)>.002;};
 for(int n=0;n<8&&!overBudget();n++){
  auto& e=nearby[refreshCursor++%nearby.size()];if(!Valid(e))continue;
  bounds(e.body,&e.bounds);RefreshShapes(e);
  auto distance=(std::min)(glm::length(wanted[0]-glm::clamp(wanted[0],e.bounds.low,e.bounds.high)),glm::length(wanted[1]-glm::clamp(wanted[1],e.bounds.low,e.bounds.high)));
  if(distance<2.f)e.seen=now;else e={};
 }
 for(int n=0;n<(std::min)(total,64)&&!overBudget();n++){
  if(cursor>=total)cursor=0;int id=current.Handle(cursor++);auto body=static_cast<std::uint8_t*>(current.Get(id));
  if(!body||body==tool||!Readable(body,0x148)||*reinterpret_cast<unsigned short*>(body+8)!=1||hasTag(body,&bone)||hasTag(body,&accessory))continue;
  DualGrab::Bounds box{};bounds(body,&box);bool sane=true;
  for(int axis=0;axis<3;axis++)if(!std::isfinite(box.low[axis])||!std::isfinite(box.high[axis])||box.low[axis]>box.high[axis])sane=false;
  if(!sane)continue;
  float distance=(std::min)(glm::length(wanted[0]-glm::clamp(wanted[0],box.low,box.high)),glm::length(wanted[1]-glm::clamp(wanted[1],box.low,box.high)));
  if(distance>2.f)continue;
  Entry* slot=&nearby[0];for(auto& e:nearby){if(e.id==id&&e.body==body){slot=&e;break;}if(e.seen<slot->seen)slot=&e;}
  auto lastPush=(slot->id==id&&slot->body==body)?slot->lastPush:0;*slot={id,body,now,lastPush,box};RefreshShapes(*slot);
 }
 float dt=previousStamp&&sample.stamp>previousStamp?float(sample.stamp-previousStamp)/1000.f:0.f;
 // Never collide with a cached box left behind by a moving body. Defer any
 // refresh beyond this update's budget instead of retaining phantom contacts.
 for(auto& e:nearby)if(Valid(e)&&std::memcmp(&e.pose,e.body+0x40,sizeof(e.pose))){if(overBudget())e.shapeCount=0;else RefreshShapes(e);}
 for(int hand=0;hand<2;hand++){
  if(!sample.valid[hand])initialized[hand]=false;
  if(dt<=.001f||dt>.2f||glm::length(wanted[hand]-previousWanted[hand])>.5f)initialized[hand]=false;
  auto rawMotion=dt>.001f&&dt<=.2f?(wanted[hand]-previousWanted[hand])/dt:glm::vec3(0);
  void* ignore=hand==0&&!handsOnly&&!SupportGrip::active?nullptr:tool;
  auto resolved=initialized[hand]?Move(previous[hand],wanted[hand],HandRadius,dt,ignore,true,&rawMotion):wanted[hand];
  previousWanted[hand]=wanted[hand];
  // The free hand can touch the equipped tool without grabbing it. Do not
  // obstruct the main hand or the latched support grip with its own tool.
  // Keep the surface origin while pressing through it; clipping the offset
  // every frame used to push that origin inside the wall and lose all hits.
  if(glm::length(resolved-wanted[hand])>.65f){resolved=wanted[hand];initialized[hand]=false;}
  previous[hand]=resolved;initialized[hand]=sample.valid[hand];
  std::lock_guard<std::mutex> guard(contactMutex);contactOffset[hand]=resolved-wanted[hand];
 }
 previousStamp=sample.stamp;std::lock_guard<std::mutex> guard(contactMutex);contactStamp=now;
}
}

void DualGrab::AimCandidate(const Entities& entities,const glm::mat4& hand,void* tool,Grip& result,glm::vec3& contact){
 auto from=glm::vec3(hand[3]);auto direction=Controllers::Aim(hand);float distance=1.5f;glm::vec3 normal;std::uint8_t* body=nullptr;
 if(!NativeContact::Ray(from,from+direction*distance,tool,distance,normal,body,true))return;
 float nearest=.035f;auto hit=from+direction*distance;
 Candidate(entities,*reinterpret_cast<int*>(body+0xC),hit,tool,nullptr,result,nearest,contact);
}
void DualGrab::NearbyCandidates(const Entities& entities,glm::vec3 pos,void* tool,void* other,Grip& result,float& nearest,glm::vec3& contact){
 LARGE_INTEGER start,freq;QueryPerformanceCounter(&start);QueryPerformanceFrequency(&freq);
 for(auto& entry:WorldContact::nearby){
  LARGE_INTEGER now;QueryPerformanceCounter(&now);if(double(now.QuadPart-start.QuadPart)/double(freq.QuadPart)>.001)break;
  if(!entry.body||entities.Get(entry.id)!=entry.body||GetTickCount64()-entry.seen>1500)continue;
  if(glm::length(pos-glm::clamp(pos,entry.bounds.low,entry.bounds.high))>nearest)continue;
  Candidate(entities,entry.id,pos,tool,other,result,nearest,contact);
 }
}
glm::mat4 ConstrainTool(void* body,const glm::mat4& desired){
 if(handsOnly||!physicalContact||menuOpen||inVehicle||WorldContact::previousStamp==0||GetTickCount64()-WorldContact::previousStamp>250)return desired;
 Transform previous;std::memcpy(&previous,static_cast<std::uint8_t*>(body)+0x40,sizeof(previous));if(!Controllers::Valid(previous))return desired;
 auto old=Controllers::Matrix(previous);WorldContact::Entry toolEntry{};toolEntry.body=static_cast<std::uint8_t*>(body);WorldContact::RefreshShapes(toolEntry);
 // Probe the center and eight bounds corners, preserving one rigid transform.
 auto result=desired;glm::vec3 correction(0),hitFrom(0),hitTo(0);
 LARGE_INTEGER start,freq;QueryPerformanceCounter(&start);QueryPerformanceFrequency(&freq);bool exhausted=false;
 const auto inverseOld=glm::inverse(old);
 for(int shape=0;shape<toolEntry.shapeCount&&!exhausted;shape++){
 auto box=toolEntry.shapes[shape];
 for(int i=0;i<9;i++){
  LARGE_INTEGER now;QueryPerformanceCounter(&now);if(double(now.QuadPart-start.QuadPart)/double(freq.QuadPart)>.001){exhausted=true;break;}
  glm::vec3 point=i==8?(box.low+box.high)*.5f:glm::vec3((i&1)?box.high.x:box.low.x,(i&2)?box.high.y:box.low.y,(i&4)?box.high.z:box.low.z);
  auto child=*reinterpret_cast<std::uint8_t**>(static_cast<std::uint8_t*>(body)+0x28);glm::vec3 surface,normal;float nearest=100.f;bool found=false;
  for(int n=0;n<32&&Readable(child,0xF8);n++,child=*reinterpret_cast<std::uint8_t**>(child+0x18)){
   LARGE_INTEGER budgetNow;QueryPerformanceCounter(&budgetNow);if(double(budgetNow.QuadPart-start.QuadPart)/double(freq.QuadPart)>.001){exhausted=true;break;}
   if(*reinterpret_cast<unsigned short*>(child+8)!=2)continue;glm::vec3 candidate,norm;
   if(NativeContact::Closest(child,point,.5f,candidate,norm)){float distance=glm::length(candidate-point);if(distance<nearest){nearest=distance;surface=candidate;found=true;}}
  }
  if(!found)continue;point=surface;
  auto local=inverseOld*glm::vec4(point,1);auto target=glm::vec3(desired*local);
  auto resolved=WorldContact::Move(point,target,WorldContact::ToolRadius,1.f/60.f,body,false);auto offset=resolved-target;
  if(glm::length(offset)>glm::length(correction)){correction=offset;hitFrom=point;hitTo=target;}
 }
 }
 if(glm::length(correction)>.0001f)WorldContact::Move(hitFrom,hitTo,WorldContact::ToolRadius,1.f/60.f,body,true);
 result[3]+=glm::vec4(GrabMath::Limit(correction,.65f),0);return result;
}
