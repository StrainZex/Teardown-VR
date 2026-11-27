#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <atomic>
#include <array>
#include <cstring>
#include <future>
#include <thread>
#include <stdexcept>
#include <iostream>
#include "snapshot_cache.h"
#include "controller_math.h"
void check(bool ok,const char* name){if(!ok)throw std::runtime_error(name);}
namespace Fixture {
using Controllers::Transform;
std::atomic<bool> enabled{true},inVehicle{false};
std::uint8_t* gameBase=nullptr;void* boundBody=nullptr;
alignas(16) std::array<std::uint8_t,0x2E00> player{};
struct Snapshot{bool input=true;};bool input=true;
unsigned lookups=0,reads=0,copies=0,logs=0;ULONGLONG now=10000;
void* LocalPlayer(){++lookups;return player.data();}
bool Readable(const void* p,std::size_t){++reads;return p!=nullptr;}
bool Copy(Snapshot& s){++copies;s.input=input;return enabled;}
void Log(const std::string&){++logs;}
ULONGLONG GetTickCount64(){return now;}
#include "avatar_pose.h"
Transform* Visual(void*,Transform* out,const Transform* current){*out=*current;out->x-=.25f;return out;}
}
int main(){try{
 struct Value{std::uint64_t stamp=0;int payload=0;};
 std::mutex mutex;Value shared{1000,42},out;
 SnapshotCache::Reader<Value> reader,empty;
 check(reader.Read(mutex,shared,true,1001,out)&&out.payload==42,"fresh snapshot");
 std::promise<void> acquired;auto ready=acquired.get_future();
 std::thread owner([&]{std::lock_guard<std::mutex> lock(mutex);acquired.set_value();Sleep(250);});
 ready.wait();auto begin=GetTickCount64();
 bool cached=reader.Read(mutex,shared,true,1100,out)&&out.payload==42;
 bool refused=!SnapshotCache::Publish(mutex,shared,Value{1100,99});
 bool absent=!empty.Read(mutex,shared,true,1100,out);
 bool expired=!reader.Read(mutex,shared,true,1250,out);
 auto elapsed=GetTickCount64()-begin;owner.join();
 check(cached&&refused&&absent&&expired&&elapsed<150,"snapshot readers/publisher must not wait for held lock");
 check(SnapshotCache::Publish(mutex,shared,Value{1300,99}),"publish after contention");
 check(reader.Read(mutex,shared,true,1301,out)&&out.payload==99,"reader catches latest pose");
 check(!reader.Read(mutex,shared,false,1301,out),"VR disabled snapshot");
 check(!reader.Read(mutex,shared,true,1299,out),"future timestamp rejected");
 using namespace Fixture;
 std::array<std::uint8_t,0x100> shape{},body{},parent{};
 auto set=[&](auto& memory,std::size_t at,auto value){std::memcpy(memory.data()+at,&value,sizeof(value));};
 set(shape,0x10,body.data());set(body,0x10,parent.data());set(parent,8,std::uint16_t(12));set(parent,0xC,42);
 set(player,0x7C8,1.f);set(player,0x820,body.data());set(player,0x2DC8,42);boundBody=body.data();
 AvatarPose::originalVisual=Visual;
 Transform current{1,2,3,0,0,0,1},visual{};
 for(int i=0;i<100;i++)AvatarPose::ResolveVisual(shape.data()+0x60,&visual,&current,0x4ea72a);
 check(visual.x==current.x&&lookups==1&&copies==1&&logs==1,"same-frame avatar pose correction must cache owner and input checks");
 ++AvatarPose::visualEpoch;AvatarPose::ResolveVisual(shape.data()+0x60,&visual,&current,0x4ea854);
 check(lookups==2&&copies==2&&logs==1,"one validation per next frame; global diagnostic limit");
 inVehicle=true;AvatarPose::ResolveVisual(shape.data()+0x60,&visual,&current,0x4ea72a);
 check(visual.x==.75f,"vehicle uses native interpolation");inVehicle=false;
 input=false;++AvatarPose::visualEpoch;AvatarPose::ResolveVisual(shape.data()+0x60,&visual,&current,0x4ea72a);
 check(visual.x==.75f,"input unavailable uses native interpolation");input=true;
 set(player,0x7C8,0.f);++AvatarPose::visualEpoch;AvatarPose::ResolveVisual(shape.data()+0x60,&visual,&current,0x4ea72a);
 check(visual.x==.75f,"dead avatar uses native interpolation");set(player,0x7C8,1.f);
 now+=101;AvatarPose::ResolveVisual(shape.data()+0x60,&visual,&current,0x4ea72a);
 check(visual.x==1,"long-frame owner validation refresh");
 boundBody=nullptr;set(player,0x820,static_cast<void*>(nullptr));set(parent,0xC,99);++AvatarPose::visualEpoch;
 AvatarPose::ResolveVisual(shape.data()+0x60,&visual,&current,0x4ea72a);check(visual.x==.75f,"other animator preserved");
 auto count=lookups;set(parent,8,std::uint16_t(1));AvatarPose::ResolveVisual(shape.data()+0x60,&visual,&current,0x4ea72a);
 check(lookups==count&&visual.x==.75f,"ordinary world body exits without owner queries");
 std::cout<<"PASS held-lock nonblocking snapshots, stale/future/focus rejection, cached actual avatar visual hook, native vehicle/dead/other-body behavior and bounded logging\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
