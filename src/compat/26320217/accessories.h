#pragma once
// Narrow Lua/native bridge. Ordinary GetPlayerHealth calls are forwarded intact.
// The companion uses a reserved numeric marker, never raw pointers from scripts.
namespace Accessories {
using LuaFn=void(*)(void*,void*,void*);LuaFn originalHealth=nullptr;
using ReadFn=float(*)(void*,int,float);using WriteFn=void(*)(void*,float);
ReadFn argumentReader=nullptr;WriteFn resultWriter=nullptr;
std::atomic<bool> gunPack{false},handsPackOnly{false};
std::atomic<ULONGLONG> heartbeat{0};
std::atomic<void*> owner{nullptr};
std::atomic<unsigned> occupied{0};std::atomic<bool> locked{false},unholster{false};
std::atomic<int> tabletHand{-1},page{0},spawnRequest{0};
std::atomic<bool> meleeRequested{false};
std::atomic<void*> meleeTool{nullptr};std::atomic<ULONGLONG> meleeStamp{0};
void* SelectedTool(){auto p=static_cast<std::uint8_t*>(LocalPlayer());if(!Readable(p,0x998))return nullptr;auto manager=*reinterpret_cast<std::uint8_t**>(p+0x990);return Readable(manager,0x38)?*reinterpret_cast<void**>(manager+0x30):nullptr;}
bool ConsumeMelee(){if(!meleeRequested.exchange(false))return false;return GetTickCount64()-meleeStamp.load()<150&&meleeTool.load()!=nullptr&&meleeTool.load()==SelectedTool();}
std::atomic<int> meleeMode{0};std::atomic<unsigned> meleeCycle{0};
std::atomic<bool> spawnVisible{false},keyboard{false};
std::mutex poseMutex;glm::mat4 tabletPose{1.f},tabletGrip{1.f};
bool Alive(){auto t=heartbeat.load();return enabled&&t&&owner==LocalPlayer()&&(GetTickCount64()-t<500||HUDMath::interactive);}
bool Locked(){return Alive()&&locked;}
unsigned Occupied(){return Alive()?occupied.load():0;}
void Health(void* script,void* args,void* results){
 auto read=argumentReader;auto write=resultWriter;
 if(!Readable(args,12)||*reinterpret_cast<int*>(static_cast<std::uint8_t*>(args)+8)<2||read(args,0,0)!=-717.f){originalHealth(script,args,results);return;}
 auto emit=[&](float v){write(results,v);};float code=read(args,1,-1);
 if(!std::isfinite(code)||code<0||code>12){emit(0);return;}int op=int(code);
 if(op==12){emit(723);emit(enabled&&gunPack?1.f:0.f);emit(handsPackOnly?1.f:0.f);return;}
 if(op==0){emit(717);emit(enabled?1.f:0.f);return;}
 if(op==1){
  if(!Focused()){emit(0);return;}
  Snapshot s;glm::mat4 hands[2];auto player=LocalPlayer();
  if(!Copy(s)||!s.input||!WorldHands(player,s,hands[0],hands[1],false)){emit(0);return;}
  Transform c;std::memcpy(&c,static_cast<std::uint8_t*>(player)+0x640,sizeof(c));
  auto pose=[&](const glm::mat4& m){auto t=Controllers::Pack(m);emit(t.x);emit(t.y);emit(t.z);emit(t.qx);emit(t.qy);emit(t.qz);emit(t.qw);};
  emit(717);pose(hands[0]);pose(hands[1]);pose(BasePose(Controllers::Matrix(c))*s.head);
  emit(s.valid[0]);emit(s.valid[1]);emit(s.buttons.leftGrab);emit(s.buttons.grab);
  emit(s.buttons.leftTrigger);emit(s.buttons.fire);emit(inVehicle);emit(menuOpen||HUDMath::interactive);
  emit(float(s.center));emit(float(heldObjects.load()));emit(physicalMelee);emit(float(s.stamp%1000000));return;
 }
 if(op==2){
  float mask=read(args,2,0),lock=read(args,3,0),hand=read(args,4,-1);
  if(!std::isfinite(mask)||mask<0||mask>3||!std::isfinite(hand)||hand< -1||hand>1){emit(0);return;}
  occupied=unsigned(mask);locked=lock==1;tabletHand=int(hand);owner=LocalPlayer();heartbeat=GetTickCount64();
  Transform t{},g{};float* a=&t.x;float* b=&g.x;for(int i=0;i<7;i++){a[i]=read(args,5+i,i==6?1.f:0.f);b[i]=read(args,12+i,i==6?1.f:0.f);}
  if(Controllers::Valid(t)&&Controllers::Valid(g)){std::lock_guard<std::mutex> guard(poseMutex);tabletPose=Controllers::Matrix(t);tabletGrip=Controllers::Matrix(g);}
  emit(unholster.exchange(false)?1.f:0.f);return;
 }
 if(op==3){float hand=read(args,2,-1);if(enabled&&(hand==0||hand==1))Haptics::Grab(int(hand));emit(1);return;}
 if(op==4){occupied=0;locked=false;tabletHand=-1;page=0;heartbeat=0;emit(1);return;}
 if(op==5){emit(float(page.load()));return;}
 if(op==6){float p=read(args,2,0);if(p>=0&&p<=2&&std::isfinite(p))page=int(p);emit(1);return;}
 if(op==7){emit(enabled?float(spawnRequest.exchange(0)):0.f);return;}
 if(op==9){Snapshot sample;emit(720);emit(enabled&&Focused()&&Copy(sample)&&sample.input?1.f:0.f);return;}
 if(op==10){Snapshot sample;auto selected=SelectedTool();bool ready=selected&&enabled&&physicalMelee&&Focused()&&!inVehicle&&!menuOpen&&!HUDMath::interactive&&!Occupied()&&!Locked()&&!heldObjects.load()&&Copy(sample)&&sample.input&&sample.valid[1];if(ready){meleeTool=selected;meleeStamp=GetTickCount64();meleeRequested=true;}emit(ready?1.f:0.f);return;}
 if(op==11){float mode=read(args,2,0);if(std::isfinite(mode)&&mode>=0&&mode<=2)meleeMode=int(mode);emit(float(meleeCycle.exchange(0)));return;}
 if(op==8){spawnVisible=enabled&&read(args,2,0)==1;keyboard=spawnVisible&&read(args,3,0)==1;emit(1);}
}
}
