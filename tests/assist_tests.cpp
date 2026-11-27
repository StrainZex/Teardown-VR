#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <openvr.h>
namespace Harness {
bool enabled=true;void* local=nullptr;
void* LocalPlayer(){return local;}
bool Readable(const void* p,size_t){return p!=nullptr;}
#include "player_assists.h"
struct VRMock {int calls[2]{};void TriggerHapticPulse(unsigned hand,unsigned,unsigned short){++calls[hand];}} runtime;
VRMock* systemVR=&runtime;
#include "haptics.h"
float ammo=7;int writes=0;
float Get(void*){return ammo;}void Set(void*,float value){ammo=value;++writes;}
int forwarded=0;const void* lastPosition=nullptr;const void* lastDirection=nullptr;
void Damage(void*,float,int,void*,const void* p,const void* d){++forwarded;lastPosition=p;lastDirection=d;}
}
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);std::cout<<"PASS "<<message<<'\n';}
int main(){try{using namespace Harness;
 vr::TrackedDeviceIndex_t ids[2]{0,1};bool valid[2]{true,true};
 Haptics::Grab(0);Haptics::Grab(0);Haptics::Flush(ids,valid,true);Haptics::Flush(ids,valid,true);
 require(runtime.calls[0]==1&&runtime.calls[1]==0,"one event batch creates one pulse, no held repeat");
 Haptics::Grab(0);Haptics::Grab(1);Haptics::Flush(ids,valid,true);
 require(runtime.calls[0]==2&&runtime.calls[1]==1,"a new grab pulses again on the correct hand");
 Haptics::Grab(1);Haptics::Flush(ids,valid,false);Haptics::Flush(ids,valid,true);
 require(runtime.calls[1]==1,"input loss discards pending haptics");
 alignas(16) unsigned char player[0xA00]{},manager[0x60]{},tool[0x40]{};void* table[9]{};
 local=player;table[7]=reinterpret_cast<void*>(&Set);table[8]=reinterpret_cast<void*>(&Get);
 *reinterpret_cast<void***>(tool)=table;*reinterpret_cast<void**>(manager+0x30)=tool;*reinterpret_cast<void**>(player+0x990)=manager;
 PlayerAssists::unlimitedAmmo=true;
 PlayerAssists::AmmoScope scope(player);ammo=6;scope.Restore(player);require(ammo==7,"native ammo replenished after update");
 PlayerAssists::unlimitedAmmo=false;writes=0;PlayerAssists::AmmoScope off(player);off.Restore(player);require(writes==0,"ammo OFF performs no writes");
 PlayerAssists::unlimitedAmmo=true;PlayerAssists::AmmoScope switched(player);*reinterpret_cast<void**>(manager+0x30)=nullptr;writes=0;switched.Restore(player);require(writes==0,"tool switch prevents stale ammo write");
 PlayerAssists::AmmoScope nullScope(nullptr);
 PlayerAssists::originalDamage=&Damage;int position=1,direction=2;
 PlayerAssists::godMode=false;PlayerAssists::Damage(player,.5f,3,nullptr,&position,&direction);
 require(forwarded==1&&lastPosition==&position&&lastDirection==&direction,"damage OFF forwards all six ABI arguments");
 PlayerAssists::godMode=true;PlayerAssists::Damage(player,.5f,3,nullptr,&position,&direction);require(forwarded==1,"God mode suppresses local native damage");
 PlayerAssists::Damage(manager,.5f,3,nullptr,&position,&direction);require(forwarded==2,"other players are not protected by local God mode");
 enabled=false;PlayerAssists::Damage(player,.5f,3,nullptr,&position,&direction);require(forwarded==3,"disabled VR restores native damage");
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
