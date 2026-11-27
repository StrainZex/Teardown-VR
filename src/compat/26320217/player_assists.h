#pragma once
namespace PlayerAssists {
std::atomic<bool> godMode{false},unlimitedAmmo{false};
using DamageFn=void(*)(void*,float,int,void*,const void*,const void*);DamageFn originalDamage=nullptr;
void Damage(void* player,float amount,int kind,void* source,const void* position,const void* direction){
 if(enabled&&godMode&&player==LocalPlayer())return;
 originalDamage(player,amount,kind,source,position,direction);
}
// The same virtual ammo accessors called by native GetToolAmmo/SetToolAmmo.
// Private Lua counters used by some mods are outside this interface.
struct AmmoScope {
 void* tool=nullptr;float amount=0;using Set=void(*)(void*,float);Set setter=nullptr;
 explicit AmmoScope(void* player){
  if(!player||!unlimitedAmmo||!enabled||player!=LocalPlayer())return;
  auto p=static_cast<std::uint8_t*>(player);auto manager=*reinterpret_cast<std::uint8_t**>(p+0x990);
  if(!Readable(manager,0x38))return;auto selected=*reinterpret_cast<void**>(manager+0x30);
  if(!Readable(selected,0x38))return;auto table=*reinterpret_cast<void***>(selected);if(!Readable(table,0x48))return;
  using Get=float(*)(void*);amount=reinterpret_cast<Get>(table[8])(selected);
  if(!std::isfinite(amount)||amount<0||amount>1000000)return;
  tool=selected;setter=reinterpret_cast<Set>(table[7]);amount=(std::max)(amount,1.f);setter(tool,amount);
 }
 void Restore(void* player){
  if(!tool)return;auto p=static_cast<std::uint8_t*>(player);auto manager=*reinterpret_cast<std::uint8_t**>(p+0x990);
  if(Readable(manager,0x38)&&*reinterpret_cast<void**>(manager+0x30)==tool&&Readable(tool,0x38))setter(tool,amount);
 }
};
}
