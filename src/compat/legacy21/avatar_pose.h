#pragma once
// The Lua world-hand getter must use the same tool transform as GetHandsHook.
// Otherwise it composes current tool-local targets with an interpolated OLD tool.
namespace AvatarPose {
using InterpolateFn=Transform*(*)(void*,Transform*);
InterpolateFn original=nullptr;
inline constexpr std::uintptr_t HandWorldReturn=0x47E1B1;
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
}

