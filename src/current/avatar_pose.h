#pragma once
// The Lua world-hand getter must use the same tool transform as GetHandsHook.
// Otherwise it composes current tool-local targets with an interpolated OLD tool.
namespace AvatarPose {
using InterpolateFn=Transform*(*)(void*,Transform*);
InterpolateFn original=nullptr;
inline constexpr std::uintptr_t HandWorldReturn=0x48a111;
Transform* Resolve(void* body,Transform* out,std::uintptr_t caller){
 auto result=original(body,out); // Maintain native history for other consumers.
 if(caller!=HandWorldReturn)return result;
 Snapshot sample;auto player=static_cast<std::uint8_t*>(LocalPlayer());
 if(!Copy(sample)||!sample.input||!Readable(player,0x828)||*reinterpret_cast<void**>(player+0x820)!=body||!Readable(body,0x5C))return result;
 Transform current;std::memcpy(&current,static_cast<std::uint8_t*>(body)+0x40,sizeof(current));
 if(!Controllers::Valid(current))return result;
 *out=current;return out;
}
Transform* Hook(void* body,Transform* out){
 auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress())-reinterpret_cast<std::uintptr_t>(gameBase);
 return Resolve(body,out,caller);
}

// Shapes have their own visual interpolation, separate from the Lua body
// getter above. Keep the local avatar and equipped tool on the same pose.
// These four guarded callers pass shape+0x60 as history, and either the body
// pose or the already-composed shape pose as current. Preserve native history
// and replace only the returned visual transform, including culling bounds.
using VisualFn=Transform*(*)(void*,Transform*,const Transform*);
VisualFn originalVisual=nullptr;
inline bool VisualCaller(std::uintptr_t caller){
 return caller==0x4ea72a||caller==0x4ea854||caller==0x4ea9a8||caller==0x4eaa65;
}
std::atomic<unsigned long long> visualEpoch{1};
std::atomic<ULONGLONG> visualReportAt{0};
struct VisualOwner {unsigned long long epoch=0;ULONGLONG stamp=0;void* tool=nullptr;int animator=0;bool active=false;};
thread_local VisualOwner visualOwner;
thread_local unsigned visualSamples=0;
thread_local float visualMaxLag=0;
Transform* ResolveVisual(void* history,Transform* out,const Transform* current,std::uintptr_t caller){
 auto result=originalVisual(history,out,current);
 if(!enabled||inVehicle||!VisualCaller(caller))return result;
 // The native callers already dereference this exact shape/body pair. Reject
 // ordinary world bodies before any player lookup, mutex or memory-page query.
 auto shape=static_cast<std::uint8_t*>(history)-0x60;
 auto body=*reinterpret_cast<std::uint8_t**>(shape+0x10);
 auto parent=*reinterpret_cast<std::uint8_t**>(body+0x10);
 if(body!=boundBody&&(!parent||*reinterpret_cast<unsigned short*>(parent+8)!=12))return result;
 auto now=GetTickCount64();auto epoch=visualEpoch.load();
 // Validate owner/input once per render frame per worker, not for every
 // avatar mesh and both eyes. Refresh also during unusually long frames.
 if(visualOwner.epoch!=epoch||now-visualOwner.stamp>=100){
  visualOwner={epoch,now,nullptr,0,false};
  auto player=static_cast<std::uint8_t*>(LocalPlayer());Snapshot sample;
  if(Readable(player,0x2DCC)&&Copy(sample)&&sample.input){
   float health;std::memcpy(&health,player+0x7C8,sizeof(health));
   if(std::isfinite(health)&&health>0){visualOwner.tool=*reinterpret_cast<void**>(player+0x820);visualOwner.animator=*reinterpret_cast<int*>(player+0x2DC8);visualOwner.active=true;}
  }
 }
 if(!visualOwner.active||!Controllers::Valid(*current))return result;
 bool tool=body==visualOwner.tool;
 int animator=visualOwner.animator;
 if(!tool&&(!parent||*reinterpret_cast<unsigned short*>(parent+8)!=12||animator==0||
            *reinterpret_cast<int*>(parent+0xC)!=animator))return result;
 if(Controllers::Valid(*result)){
  float gap=glm::length(glm::vec3(result->x-current->x,result->y-current->y,result->z-current->z));
  if(std::isfinite(gap))visualMaxLag=(std::max)(visualMaxLag,gap);
 }
 *out=*current;++visualSamples;
 auto report=visualReportAt.load();
 if(now>=report&&visualReportAt.compare_exchange_strong(report,now+5000)){
  Log("Avatar/tool visual interpolation bypass: samples="+std::to_string(visualSamples)+" max-offset-m="+std::to_string(visualMaxLag));
  visualSamples=0;visualMaxLag=0;
 }
 return out;
}
Transform* VisualHook(void* history,Transform* out,const Transform* current){
 auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress())-reinterpret_cast<std::uintptr_t>(gameBase);
 return ResolveVisual(history,out,current,caller);
}
}

