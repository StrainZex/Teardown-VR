#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <atomic>
#include <mutex>
#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include "contact_math.h"
namespace Test {
using Controllers::Transform;
std::vector<std::uint8_t> executable(0x1C96E80);std::uint8_t* gameBase=executable.data();
std::uint8_t game[0x200]{},scene[0xA40]{},player[0xAB0]{},body[0x160]{},shape[0x110]{};
void* table[4]{};bool emptySurface=false;int wakes=0,impulses=0;glm::vec3 lastImpulse(0);
std::atomic<bool> menuOpen{false},separateGrab{true},inVehicle{false},physicalContact{true},handsOnly{true};
std::atomic<unsigned> heldObjects{0};std::mutex contactMutex;glm::vec3 contactOffset[2]{};ULONGLONG contactStamp=0;
struct Snapshot {glm::mat4 hand[2]{glm::mat4(1),glm::mat4(1)};bool input=true,valid[2]{true,true};Controllers::Buttons buttons{};ULONGLONG stamp=100;};
struct TagName {explicit TagName(const char*){}};
bool hasTag(void*,const TagName*){return false;}
bool Readable(const void* p,size_t){return p!=nullptr;}
void Log(const std::string&){}
bool WorldHands(void*,const Snapshot& s,glm::mat4& l,glm::mat4& r,bool=true){l=s.hand[0];r=s.hand[1];return true;}
namespace Accessories {unsigned Occupied(){return 0;}}
namespace SupportGrip {bool active=false;}
namespace Haptics {int pulses[2]{};void Grab(int h){pulses[h]++;}}
namespace NativeContact {bool Closest(void*,glm::vec3,float,glm::vec3&,glm::vec3&);bool Ray(glm::vec3,glm::vec3,void*,float&,glm::vec3&,std::uint8_t*&,bool=false);}
#include "dual_grab.h"
#include "world_contact.h"
bool planeEnabled=false,thin=false;
namespace NativeContact {
bool Closest(void* ptr,glm::vec3 from,float reach,glm::vec3& point,glm::vec3& normal){
 if(emptySurface)return false;DualGrab::Bounds b;std::memcpy(&b,static_cast<std::uint8_t*>(ptr)+0xA4,sizeof(b));point=glm::clamp(from,b.low,b.high);normal=glm::vec3(0,1,0);
 if(point==from){float nearest=1000;int axis=0;float face=b.low.x;for(int a=0;a<3;a++)for(float v:{b.low[a],b.high[a]})if(std::abs(from[a]-v)<nearest){nearest=std::abs(from[a]-v);axis=a;face=v;}point[axis]=face;}
 return glm::length(point-from)<reach;
}
bool Ray(glm::vec3 from,glm::vec3 to,void* ignore,float& distance,glm::vec3& normal,std::uint8_t*& hit,bool includeHeld){
 if(!planeEnabled||ignore==body)return false;
 if(!includeHeld&&(DualGrab::grip[0].body==body||DualGrab::grip[1].body==body))return false;
 float delta=to.x-from.x;if(delta<=0||from.x>.05f||to.x<.05f)return false;
 float t=(.05f-from.x)/delta;auto at=from+(to-from)*t;
 if(thin&&(at.y<.012f||at.y>.025f))return false;
 distance=glm::length(to-from)*t;normal=glm::vec3(-1,0,0);hit=body;return true;
}
}
void* Bounds(void*,DualGrab::Bounds* b){std::memcpy(b,shape+0xA4,sizeof(*b));return b;}
void Wake(void*,bool){++wakes;}
void Impulse(void*,const glm::vec3*,const glm::vec3* v,float,bool){++impulses;lastImpulse=*v;}
template<class T>void Put(void* p,size_t offset,T value){std::memcpy(static_cast<std::uint8_t*>(p)+offset,&value,sizeof(value));}
glm::vec3 Velocity(){glm::vec3 v;std::memcpy(&v,body+0xD8,12);return v;}
void Reset(){
 DualGrab::grip[0]={};DualGrab::grip[1]={};WorldContact::nearby={};WorldContact::pushed={};WorldContact::initialized[0]=WorldContact::initialized[1]=false;WorldContact::previousStamp=0;
 Haptics::pulses[0]=Haptics::pulses[1]=0;heldObjects=0;wakes=impulses=0;planeEnabled=false;emptySurface=false;thin=false;
 Put(body,0xD8,glm::vec3(0));Put(body,0xE4,glm::vec3(0));
}
void Init(){
 Put(gameBase,0x1C96E70,game);Put(game,0x50,scene);table[1]=body;table[2]=shape;
 Put(scene,0xA18,3);Put(scene,0xA20,table);Put(scene,0x208,0);
 Put(body,8,(unsigned short)1);Put(body,0xC,1);Put(body,0x28,shape);Put(body,0x40,Transform{0,0,0,0,0,0,1});Put(body,0x138,(unsigned char)1);Put(body,0xFC,1.f);
 Put(shape,8,(unsigned short)2);Put(shape,0xC,2);Put(shape,0x10,body);Put(shape,0xA4,DualGrab::Bounds{glm::vec3(-.1f),glm::vec3(.05f,.1f,.1f)});Put(player,0x970,shape);
 DualGrab::boundsFunction=Bounds;DualGrab::wakeFunction=Wake;WorldContact::impulseFunction=Impulse;
}
}
int main(){try{using namespace Test;int count=0;auto check=[&](bool ok,const char* label){if(!ok)throw std::runtime_error(label);++count;std::cout<<"PASS "<<label<<'\n';};Init();Reset();
 Snapshot s;s.hand[0][3]=glm::vec4(-.12f,0,0,1);s.hand[1][3]=glm::vec4(.12f,0,0,1);s.buttons.grab=true;
 DualGrab::Tick(player,s,true);check(heldObjects==2&&Haptics::pulses[1]==1,"right grip acquires object surface once");
 s.buttons.leftGrab=true;s.stamp+=16;wakes=0;DualGrab::Tick(player,s,true);check(heldObjects==3&&DualGrab::grip[0].body==DualGrab::grip[1].body&&wakes==1,"both hands share one body and one solver write");
 s.buttons.grab=false;s.stamp+=16;wakes=0;DualGrab::Tick(player,s,true);check(heldObjects==1&&wakes==1,"right release keeps left ownership without a release impulse");
 s.buttons.grab=true;s.stamp+=16;DualGrab::Tick(player,s,true);check(heldObjects==3,"released hand reacquires the same object");
 s.buttons.leftGrab=false;s.stamp+=16;wakes=0;DualGrab::Tick(player,s,true);check(heldObjects==2&&wakes==1,"left release keeps right ownership");
 s.buttons.leftGrab=true;s.stamp+=16;DualGrab::Tick(player,s,true);s.buttons.fire=true;s.stamp+=16;DualGrab::Tick(player,s,true);
 s.buttons.fire=false;s.stamp+=200;wakes=0;DualGrab::Tick(player,s,true);check(heldObjects==1&&wakes==1&&glm::length(Velocity())<2,"charged throw cannot launch object out of other hand");
 s.buttons.leftTrigger=true;s.stamp+=16;DualGrab::Tick(player,s,true);s.buttons.leftTrigger=false;s.stamp+=500;DualGrab::Tick(player,s,true);check(heldObjects==0&&glm::length(Velocity())>3,"last hand can throw on trigger release");
 Reset();emptySurface=true;s={};s.buttons.grab=true;DualGrab::Tick(player,s,true);check(heldObjects==0,"empty space inside broad-phase bounds is not grabbable");
 Reset();planeEnabled=true;s={};s.hand[1]=glm::rotate(glm::mat4(1),glm::radians(-90.f),glm::vec3(0,1,0));s.hand[1][3]=glm::vec4(-1,0,0,1);s.buttons.grab=true;DualGrab::Tick(player,s,true);check(heldObjects==2&&glm::length(DualGrab::grip[1].handOffset)>.9f&&glm::length(Velocity())<.001f,"pointed grab preserves object distance instead of snapping to hand");
 s.hand[0]=s.hand[1];s.buttons.leftGrab=true;s.stamp+=16;DualGrab::Tick(player,s,true);check(heldObjects==3,"pointed ray can acquire the body already held by other hand");
 Reset();planeEnabled=true;auto p=WorldContact::Move({0,0,0},{.1f,0,0},WorldContact::HandRadius,.016f,nullptr,true);check(p.x<.05f&&impulses==1&&glm::length(lastImpulse)<=2.001f,"hand contacts surface and applies a capped physical impulse");
 WorldContact::Move({0,0,0},{.1f,0,0},WorldContact::HandRadius,.016f,nullptr,true);check(impulses==1,"repeated contacts share per-body impulse cooldown");
 auto slide=WorldContact::Move({0,0,0},{.1f,.15f,0},WorldContact::HandRadius,.016f,nullptr,false);check(slide.x<.05f&&std::abs(slide.y-.15f)<.002f,"wall contact preserves lateral hand motion");
 thin=true;auto edge=WorldContact::Move({0,0,0},{.1f,0,0},WorldContact::HandRadius,.016f,nullptr,false);check(edge.x<.05f,"palm edge contacts thin geometry missed by centre ray");
 thin=false;auto pushed=WorldContact::Move(p,{.3f,0,0},WorldContact::HandRadius,.016f,nullptr,false);check(pushed.x<.05f,"continued pressure keeps sweep origin outside wall");
 auto away=WorldContact::Move(p,{-.1f,0,0},WorldContact::HandRadius,.016f,nullptr,false);check(away.x==-.1f,"pulling hand away releases contact");
 Reset();planeEnabled=true;s={};s.stamp=GetTickCount64();WorldContact::Tick(player,s,true);s.hand[0][3].x=.1f;s.stamp+=16;WorldContact::Tick(player,s,true);
 check(contactOffset[0].x<-.05f,"real contact tick exports resolved hand offset");
 s.hand[0][3].x=.3f;s.stamp+=16;WorldContact::Tick(player,s,true);check(contactOffset[0].x<-.25f,"deep pressure no longer clips the sweep origin into wall");
 auto saved=contactOffset[0];WorldContact::Tick(player,s,true);check(glm::length(saved-contactOffset[0])<.0001f,"duplicate tracking sample preserves contact");
 s.hand[0][3].x=.35f;s.stamp+=125;WorldContact::Tick(player,s,true);check(contactOffset[0].x<-.30f,"contact remains active at an eight-FPS tracking cadence");
 WorldContact::Tick(player,s,false);check(contactStamp==0,"focus or VR loss clears contact override");
 std::uint8_t tool[0x160]{},toolShape[0x110]{};Put(tool,0x28,toolShape);Put(tool,0x40,Transform{0,0,0,0,0,0,1});Put(toolShape,8,(unsigned short)2);Put(toolShape,0xA4,DualGrab::Bounds{{-.2f,-.02f,-.02f},{-.1f,.02f,.02f}});
 handsOnly=false;WorldContact::previousStamp=GetTickCount64();auto toolContact=ConstrainTool(tool,glm::translate(glm::mat4(1),glm::vec3(.4f,0,0)));
 check(toolContact[3].x<.16f,"tool sweep prevents deep penetration instead of clipping correction at twelve centimetres");
 std::cout<<count<<" actual grab/contact-path checks passed with mocked engine APIs\n";return 0;
 }catch(std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
