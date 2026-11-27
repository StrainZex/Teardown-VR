#include "contact_math.h"
#include "camera_adjust.h"
#include "menu_math.h"
#include "stereo_camera.h"
#include "stereo_sprites.h"
#include "current_build.h"
#include "controller_math.h"
#include "grab_math.h"
#include "hud_math.h"
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <vector>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <Windows.h>
std::vector<std::uint8_t> Image(const char* path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Missing fixture");std::vector<std::uint8_t>b((std::istreambuf_iterator<char>(f)),{});auto d=reinterpret_cast<IMAGE_DOS_HEADER*>(b.data());auto n=reinterpret_cast<IMAGE_NT_HEADERS64*>(b.data()+d->e_lfanew);std::vector<std::uint8_t>v(n->OptionalHeader.SizeOfImage);std::memcpy(v.data(),b.data(),n->OptionalHeader.SizeOfHeaders);auto s=IMAGE_FIRST_SECTION(n);for(unsigned i=0;i<n->FileHeader.NumberOfSections;++i)std::memcpy(v.data()+s[i].VirtualAddress,b.data()+s[i].PointerToRawData,s[i].SizeOfRawData);return v;}
int main(int argc,char**argv){try{if(argc!=1&&argc!=3)return 2;int count=0;auto require=[&](bool b,const char*n){if(!b)throw std::runtime_error(n);std::cout<<"PASS "<<n<<'\n';++count;};
 std::vector<std::uint8_t> old,now;
 if(argc==3){old=Image(argv[1]);now=Image(argv[2]);}
 else {old.resize(CurrentBuild::ImageSize);now.resize(CurrentBuild::ImageSize);for(auto&s:CurrentBuild::Sites)std::memcpy(now.data()+s.rva,s.bytes.data(),s.bytes.size());std::cout<<"Synthetic signature fixture; real game compatibility not tested.\n";}
require(CurrentBuild::Matches(now.data(),now.size()),"expected signature table accepted");require(!CurrentBuild::Matches(old.data(),old.size()),"nonmatching signature table rejected");require(!CurrentBuild::Matches(nullptr,now.size()),"null rejected");require(!CurrentBuild::Matches(now.data(),now.size()-1),"wrong image size rejected");for(auto&s:CurrentBuild::Sites){auto b=now;b[s.rva]^=1;require(!CurrentBuild::Matches(b.data(),b.size()),s.name);}
 Stereo::Camera c{glm::perspective(glm::radians(80.f),1.f,0.2f,500.f),glm::mat4(1.f),0.2f,500.f};require(Stereo::Valid(c),"valid camera accepted");auto invalid=c;invalid.nearClip=0;require(!Stereo::Valid(invalid),"zero near plane rejected");invalid=c;invalid.view[0][0]=std::numeric_limits<float>::quiet_NaN();require(!Stereo::Valid(invalid),"NaN camera rejected");invalid=c;invalid.view=glm::mat4(0.f);require(!Stereo::Valid(invalid),"singular camera rejected");
 vr::HmdMatrix44_t p{};for(int row=0;row<4;row++)for(int col=0;col<4;col++)p.m[row][col]=c.projection[col][row];
 auto left=Stereo::EyeCamera(c,glm::mat4(1.f),glm::translate(glm::mat4(1.f),glm::vec3(-.032f,0,0)),p);auto right=Stereo::EyeCamera(c,glm::mat4(1.f),glm::translate(glm::mat4(1.f),glm::vec3(.032f,0,0)),p);
 auto lp=glm::inverse(left.view)[3],rp=glm::inverse(right.view)[3];require(std::abs(rp.x-lp.x-.064f)<.00001f,"64mm eye separation preserved");require(left.projection[2][2]==c.projection[2][2]&&left.projection[3][2]==c.projection[3][2],"game depth convention preserved");require(Stereo::Valid(left)&&Stereo::Valid(right),"both eye cameras valid");
 require(std::abs(left.halfVerticalFov-glm::radians(40.f))<.00001f&&left.halfVerticalFov==right.halfVerticalFov,"new camera FOV field matches both eye projections");
 auto wideProjection=p;wideProjection.m[1][1]/=1.35f;wideProjection.m[0][2]=.12f;
 auto wideCamera=Stereo::EyeCamera(c,glm::mat4(1.f),glm::mat4(1.f),wideProjection);
 require(wideCamera.halfVerticalFov>left.halfVerticalFov&&wideCamera.projection[2][0]==.12f,"wide asymmetric eye projection updates renderer FOV");
 invalid=c;invalid.halfVerticalFov=std::numeric_limits<float>::quiet_NaN();require(!Stereo::Valid(invalid),"invalid new camera field rejected");
 auto translated=Stereo::EyeCamera(c,glm::translate(glm::mat4(1.f),glm::vec3(.25f,.1f,0)),glm::mat4(1.f),p);require(std::abs(glm::inverse(translated.view)[3].x-.25f)<.00001f,"head translation preserved");
 vr::HmdMatrix34_t pose{{{1,0,0,3},{0,1,0,4},{0,0,1,5}}};auto converted=Stereo::Pose(pose);require(converted[3]==glm::vec4(3,4,5,1),"OpenVR row-major pose converted");
 auto heading=glm::rotate(glm::mat4(1.f),glm::radians(40.f),glm::vec3(0,1,0));
 auto tilted=glm::rotate(glm::rotate(heading,glm::radians(30.f),glm::vec3(1,0,0)),glm::radians(25.f),glm::vec3(0,0,1));tilted[3]=glm::vec4(2,1.7f,3,1);
 auto upright=Stereo::Upright(tilted);
 require(glm::length(upright[1]-glm::vec4(0,1,0,0))<.00001f,"centering preserves gravity despite initial tilt");
 require(upright[3]==tilted[3],"centering uses current head position");
 require(glm::length(upright[2]-heading[2])<.00001f,"centering retains yaw only");
 auto headLevel=heading;headLevel[3]=tilted[3];auto relative=glm::inverse(upright)*headLevel;
 require(glm::length(relative[1]-glm::vec4(0,1,0,0))<.00001f,"level head has no residual initial roll");
 require(glm::length(glm::vec3(relative[3]))<.00001f,"recenter removes room position offset");
 auto aimed=c;aimed.view=glm::inverse(tilted);auto leveledEye=Stereo::EyeCamera(aimed,glm::mat4(1.f),glm::mat4(1.f),p);auto leveledPose=glm::inverse(leveledEye.view);
 require(glm::length(leveledPose[1]-glm::vec4(0,1,0,0))<.00001f,"game pitch and roll do not tilt VR world");
 require(glm::length(leveledPose[3]-tilted[3])<.00001f,"game camera position retained");
 auto vertical=glm::rotate(glm::mat4(1.f),glm::radians(90.f),glm::vec3(1,0,0));require(std::abs(glm::determinant(Stereo::Upright(vertical))-1.f)<.00001f,"vertical gaze has valid upright fallback");
 auto handWorld=glm::translate(glm::mat4(1.f),glm::vec3(3,2,-4))*heading;
 Controllers::Transform grip{.1f,-.2f,.05f,0,0,0,1};auto toolWorld=Controllers::ToolAtGrip(handWorld,grip);
 require(glm::length((toolWorld*Controllers::Matrix(grip))[3]-handWorld[3])<.00001f,"tool grip reaches tracked right hand");
 // Modded models may author their forward axis as +X or +Z. Keep that
 // correction while moving their grip to the tracked controller.
 for(float angle:{90.f,180.f}){
  auto authoredMatrix=heading*glm::rotate(glm::mat4(1.f),glm::radians(angle),glm::vec3(0,1,0));
  auto authored=Controllers::Pack(authoredMatrix);auto bone=Controllers::Pack(glm::rotate(glm::mat4(1.f),glm::radians(90.f),glm::vec3(0,0,1)));bone.x=.2f;
  Controllers::HandPose hp{bone,true,{}};
  auto posed=Controllers::AuthoredToolAtHand(handWorld,heading,authored,hp);
  auto expected=handWorld*glm::rotate(glm::mat4(1.f),glm::radians(angle),glm::vec3(0,1,0));
  require(glm::length(posed[2]-expected[2])<.0001f,"modded model axis correction preserved");
  require(glm::length((posed*Controllers::Matrix(bone))[3]-handWorld[3])<.0001f,"authored grip reaches controller without bone-axis inversion");
 }
 Controllers::HandPose badGrip{{0,500,0,0,0,0,1},true,{}};
 auto bounded=Controllers::AuthoredToolAtHand(handWorld,heading,Controllers::Pack(heading),badGrip);
 require(glm::length(bounded[3]-handWorld[3])<.0001f,"outlier hand offsets cannot put tool in sky");
 require(Controllers::NextTurnSpeed(150)==180&&Controllers::NextTurnSpeed(240)==360&&Controllers::NextTurnSpeed(360)==45,"smooth turn range includes faster speeds and wraps");
 auto packed=Controllers::Pack(handWorld);require(Controllers::Valid(packed),"packed controller transform valid");
 auto roundtrip=Controllers::Matrix(packed);require(glm::length(roundtrip[2]-handWorld[2])<.00001f&&glm::length(roundtrip[3]-handWorld[3])<.00001f,"native quaternion layout roundtrips pose");
 auto aim=Controllers::Aim(handWorld);require(std::abs(glm::length(aim)-1)<.00001f&&glm::length(aim+glm::vec3(heading[2]))<.00001f,"controller ray points along tracked negative Z");
 auto otherHand=glm::translate(handWorld,glm::vec3(-.5f,0,0));auto leftLocal=Controllers::Pack(glm::inverse(toolWorld)*otherHand);
 require(glm::length((toolWorld*Controllers::Matrix(leftLocal))[3]-otherHand[3])<.00001f,"left hand resolves independently in tool space");
 vr::VRControllerState_t ls{},rs{};auto buttons=Controllers::Map(ls,rs);require(!buttons.forward&&!buttons.fire&&!buttons.grab&&buttons.turn==0,"neutral controllers release inputs");
 ls.rAxis[0].y=.8f;ls.rAxis[0].x=-.8f;rs.rAxis[0].x=.9f;rs.rAxis[1].x=.9f;rs.rAxis[2].x=.8f;
 buttons=Controllers::Map(ls,rs);require(buttons.forward&&buttons.left&&!buttons.back&&!buttons.right,"left stick movement directions");require(buttons.fire&&buttons.grab&&buttons.turn>.8f,"trigger grip and turn map");
 ls={};rs={};ls.rAxis[0].x=.2f;rs.rAxis[0].x=.2f;buttons=Controllers::Map(ls,rs);require(!buttons.right&&buttons.turn==0,"stick deadzones suppress drift");
 ls.ulButtonPressed=vr::ButtonMaskFromId(vr::k_EButton_A)|vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu);rs.ulButtonPressed=ls.ulButtonPressed;buttons=Controllers::Map(ls,rs);require(buttons.jump&&buttons.menu&&!buttons.crouch&&buttons.interact&&buttons.light,"B opens menu instead of crouching");
 ls.ulButtonPressed=vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad);rs.ulButtonPressed=0;buttons=Controllers::Map(ls,rs);require(buttons.sprint&&!buttons.crouch,"left stick click sprints");
 rs.ulButtonPressed=ls.ulButtonPressed;ls.ulButtonPressed=0;buttons=Controllers::Map(ls,rs);require(!buttons.sprint&&buttons.crouch,"right stick click crouches");
 ls.ulButtonPressed=rs.ulButtonPressed;buttons=Controllers::Map(ls,rs);require(!buttons.sprint&&!buttons.crouch,"recenter chord suppresses sprint and crouch");
 rs.rAxis[0].y=.9f;require(Controllers::Map(ls,rs).wheel==1,"tool cycle up");rs.rAxis[0].y=-.9f;require(Controllers::Map(ls,rs).wheel==-1,"tool cycle down");
 ls={};rs={};ls.rAxis[2].x=.9f;buttons=Controllers::Map(ls,rs);require(buttons.leftGrab&&!buttons.grab,"left grip supports native grab independently of right button");
 require(CurrentBuild::WeaponCameraCaller(0x44f313),"built-in weapon camera call uses tracked aiming");
 require(!CurrentBuild::WeaponCameraCaller(0x443283),"previous build camera call is not redirected");
 require(CurrentBuild::WeaponAimCaller(0x44ff92)&&!CurrentBuild::WeaponAimCaller(0x450000),"weapon aim redirects verified callers only");
 require(!CurrentBuild::WeaponCameraCaller(0x478C7A)&&!CurrentBuild::WeaponCameraCaller(0xC5113),"ordinary camera and player updates retain head camera");
 auto fixedHand=glm::rotate(glm::mat4(1.f),glm::radians(-70.f),glm::vec3(0,1,0));
 auto modelCorrection=glm::rotate(glm::mat4(1.f),glm::radians(-90.f),glm::vec3(1,0,0));
 for(float gaze:{0.f,80.f}){
  auto headCamera=glm::rotate(glm::mat4(1.f),glm::radians(gaze),glm::vec3(0,1,0));
  auto placed=Controllers::AuthoredToolAtHand(fixedHand,headCamera,Controllers::Pack(headCamera*modelCorrection),Controllers::HandPose{});
  auto barrelAim=glm::normalize(glm::mat3(placed)*glm::vec3(0,1,0));
  require(glm::length(barrelAim-Controllers::Aim(fixedHand))<.0001f,"head rotation does not steer model barrel or controller shot ray");
 }
 Controllers::XButtonGesture gesture;
 require(!gesture.Update(true,true,100,0).reload&&gesture.Update(false,true,200,0).reload,"X tap reloads on release");
 gesture.Update(true,true,1000);require(!gesture.Update(true,true,1999).interact&&gesture.Update(true,true,2000).interact,"X interaction requires 1 second");
 require(!gesture.Update(true,true,4000).interact&&!gesture.Update(false,true,4100).reload,"long X emits one interact and no reload");
 gesture.Update(true,true,5000);gesture.Update(false,false,5010);require(!gesture.Update(false,true,5020).reload,"focus loss cancels X gesture");
 auto hud=HUDMath::Viewport(1920,1080,1.f,1.2f,.15f,.05f,-.032f);
 require(hud[0]>0&&hud[1]>0&&hud[0]+hud[2]<1920&&hud[1]+hud[3]<1080,"bottom HUD stays inside central field of view");
 require(glm::length(GrabMath::FollowVelocity(glm::vec3(0),glm::vec3(.1f,0,0),glm::vec3(0))-glm::vec3(1.8f,0,0))<.0001f,"grab follows its own hand position");
 require(glm::length(GrabMath::Limit(glm::vec3(100,0,0),12))<12.001f,"throw speed bounded on tracking jumps");
 require(glm::length(GrabMath::AngularVelocity(glm::mat4(1),glm::mat4(1)))<.0001f,"matching grip orientation does not spin body");
 gesture.Update(true,true,6000);require(gesture.Update(false,true,7001).interact,"long X release remains interaction after delayed input tick");
 GrabMath::TriggerThrow charge,otherCharge;
 require(charge.Update(true,true,100)==0&&charge.Update(true,true,600)==0,"trigger holds charge without throwing");
 require(std::abs(charge.Update(false,true,600)-7.5f)<.001f,"trigger release launches charged throw");
 require(charge.Update(false,true,700)==0,"throw fires once per trigger release");
 charge.Update(true,true,1000);require(charge.Update(false,true,4000)==12.f,"throw charge has bounded maximum");
 charge.Update(true,true,5000);charge.Update(true,false,5100);require(charge.Update(false,true,5200)==0,"lost object cancels throw charge");
 charge.Update(true,true,6000);require(otherCharge.Update(false,true,6200)==0&&charge.Update(false,true,6200)>0,"hands charge throws independently");
 ls={};rs={};ls.rAxis[1].x=.9f;require(Controllers::Map(ls,rs).leftTrigger&&!Controllers::Map(ls,rs).fire,"left trigger mapped independently of weapon fire");
 for(float angle:{0.f,45.f,170.f}){
  auto controller=glm::translate(glm::rotate(glm::mat4(1),glm::radians(angle),glm::vec3(0,1,0)),glm::vec3(.2f,-.1f,-.4f));
  auto wrist=Controllers::AnimationHand(controller);
  require(glm::length(glm::vec3(wrist[3]-controller[3]))<.0001f,"wrist correction preserves tracked attachment position");
  require(glm::length(glm::vec3(wrist[0])-Controllers::Aim(controller))<.0001f,"native hand axis points along controller aim");
  Controllers::HandPose handle{Controllers::Pack(glm::translate(glm::mat4(1),glm::vec3(0,0,-.15f))),true};
  auto attached=Controllers::AuthoredToolAtHand(controller,glm::mat4(1),Controllers::Pack(glm::mat4(1)),handle);
  auto localHand=glm::inverse(attached)*wrist;
  require(glm::length(glm::vec3(localHand[3])-glm::vec3(0,0,-.15f))<.0001f,"shotgun authored handle and corrected wrist remain attached");
 }
 Controllers::HandPose invalidGrip{};require(!Controllers::UsableGrip(invalidGrip),"missing grip requests authored fallback");
 invalidGrip.used=true;invalidGrip.transform={0,0,0,0,0,0,1};require(Controllers::UsableGrip(invalidGrip),"valid authored grip accepted");
 invalidGrip.transform.x=10;require(!Controllers::UsableGrip(invalidGrip),"outlier grip rejected");
 auto raisedHud=HUDMath::Viewport(1920,1080,1.f,1.2f,0.f,0.f,0.f);
 require(raisedHud[1]+raisedHud[3]/2<int(1080*.48f),"HUD lowered below center");
 Controllers::Buttons driving{};driving.fire=true;driving.turn=1;driving.wheel=1;
 auto mapped=Controllers::VehicleButtons(driving);require(mapped.forward&&!mapped.back&&!mapped.fire&&mapped.turn==1&&mapped.wheel==1,"vehicle triggers preserve right stick look and zoom");
 driving={};driving.leftTrigger=true;driving.grab=true;driving.leftGrab=true;driving.jump=true;
 mapped=Controllers::VehicleButtons(driving);require(mapped.back&&!mapped.forward&&mapped.fire&&mapped.grab&&mapped.jump,"vehicle reverse grips and brake mapped independently");
 glm::vec3 primary(0),support(0,0,-.4f),origin(2,1,3),secondary(2.4f,1,3);
 auto two=ContactMath::TwoHand(glm::mat4(1),primary,support,origin,secondary);
 require(glm::length(glm::vec3(two*glm::vec4(primary,1))-origin)<.0001f,"two-hand pose retains primary handle");
 require(glm::length(glm::vec3(two*glm::vec4(support,1))-secondary)<.0001f,"support grip rotates tool toward second hand");
 auto reverse=ContactMath::TwoHand(glm::mat4(1),primary,support,origin,origin+glm::vec3(0,0,.4f));
 require(Controllers::Valid(Controllers::Pack(reverse)),"opposite two-hand direction produces finite rotation");
 float contactTime=0;glm::vec3 normal;
 require(ContactMath::Sweep(glm::vec3(-2,0,0),glm::vec3(2,0,0),glm::vec3(-.5f),glm::vec3(.5f),.05f,contactTime,normal)&&contactTime<.4f&&normal.x==-1,"fast hand sweep contacts wall without tunneling");
 require(!ContactMath::Sweep(glm::vec3(-2,2,0),glm::vec3(2,2,0),glm::vec3(-.5f),glm::vec3(.5f),.05f,contactTime,normal),"hand moving past surface is unobstructed");
 require(!ContactMath::Sweep(glm::vec3(0),glm::vec3(2,0,0),glm::vec3(-.5f),glm::vec3(.5f),.05f,contactTime,normal),"starting overlap permits recovery after recenter");
 require(glm::length(ContactMath::GentlePush(glm::vec3(0),glm::vec3(0),glm::vec3(-1,0,0)))==0,"stationary hand cannot build push force from penetration");
 require(glm::length(ContactMath::GentlePush(glm::vec3(50,0,0),glm::vec3(0),glm::vec3(-1,0,0)))<=.08001f,"fast contact has gentle impulse cap");
 require(glm::length(ContactMath::GentlePush(glm::vec3(.2f,0,0),glm::vec3(.3f,0,0),glm::vec3(-1,0,0)))==0,"object moving away receives no extra acceleration");
 Controllers::XButtonGesture prompt;
 require(prompt.Update(true,true,100,1,123).interact,"ordinary prompt interacts immediately on X press");
 require(!prompt.Update(false,true,150,1,123).reload,"prompt release does not reload");
 prompt.Update(true,true,200,2,321);require(!prompt.Update(false,true,300,2,321).interact,"short vehicle X press cannot enter");
 prompt.Update(true,true,400,2,321);require(!prompt.Update(true,true,1400,2,654).interact,"changing vehicle target cancels held entry");
 require(glm::length(ContactMath::GentlePush(glm::vec3(0),glm::vec3(0),glm::vec3(-1,0,0),.1f))>0,"pressing against a body produces bounded pressure push");
 float nearTime;glm::vec3 nearNormal;
 require(!ContactMath::Sweep(glm::vec3(-.1f,0,0),glm::vec3(-.005f,0,0),glm::vec3(0,-1,-1),glm::vec3(1),.004f,nearTime,nearNormal),"hand stays free five millimeters before a surface");
 require(ContactMath::Sweep(glm::vec3(-.1f,0,0),glm::vec3(.01f,0,0),glm::vec3(0,-1,-1),glm::vec3(1),.004f,nearTime,nearNormal),"smaller hand collider still contacts the surface");
 require(!ContactMath::Sweep(glm::vec3(-.1f,0,0),glm::vec3(-.003f,0,0),glm::vec3(0,-1,-1),glm::vec3(1),.002f,nearTime,nearNormal),"tool stays free three millimeters before a surface");
 glm::vec3 low(-1),high(1);ContactMath::InsetBounds(low,high);
 require(std::abs(low.x+.985f)<.00001f&&std::abs(high.x-.985f)<.00001f,"contact boxes inset by 15 mm at most");
 low=glm::vec3(0);high=glm::vec3(.002f);ContactMath::InsetBounds(low,high);
 require(high.x>low.x&&std::abs(high.x-low.x-.0012f)<.00001f,"thin contact boxes retain positive thickness");
 LARGE_INTEGER benchStart,benchEnd,benchFrequency;QueryPerformanceFrequency(&benchFrequency);QueryPerformanceCounter(&benchStart);int hits=0;
 for(int i=0;i<100000;i++){float at;glm::vec3 n;float y=float(i%100)*.02f;hits+=ContactMath::Sweep(glm::vec3(-2,y,0),glm::vec3(2,y,0),glm::vec3(-.5f),glm::vec3(.5f),.004f,at,n)?1:0;}
 QueryPerformanceCounter(&benchEnd);require(hits==26000,"contact stress sweep preserves hit and miss results");
 std::cout<<"100000 contact sweeps ms="<<1000.0*(benchEnd.QuadPart-benchStart.QuadPart)/benchFrequency.QuadPart<<'\n';
 Controllers::WheelRepeat wheel;
 require(wheel.Update(1,true,100),"vehicle zoom begins on stick press");
 require(!wheel.Update(1,true,150)&&wheel.Update(1,true,200),"held vehicle zoom repeats at 100 ms intervals");
 require(!wheel.Update(0,true,210)&&!wheel.Update(0,true,500),"releasing vehicle zoom stops repeat");
 require(wheel.Update(-1,true,510),"vehicle zoom reverses immediately");
 wheel={};require(wheel.Update(1,true,100)&&wheel.Update(1,true,1000)&&!wheel.Update(0,true,1010),"on-foot tool scrolling repeats until stick release");
 require(Controllers::MenuScroll(.8f,0)==1&&Controllers::MenuScroll(0,-.8f)==-1,"either stick scrolls native menus in both directions");
 require(Controllers::MenuScroll(.2f,-.2f)==0,"menu stick deadzone suppresses drift");
 require(Controllers::MenuScroll(.9f,-.4f)==1,"stronger stick wins instead of doubling scroll events");
 Controllers::Buttons alt{};alt.leftTrigger=true;
 require(Controllers::SecondaryAction(alt,false,false,true,0),"left trigger restores alternate fire with separate grips");
 require(!Controllers::SecondaryAction(alt,false,false,true,1),"left-hand throw charge suppresses alternate fire");
 require(Controllers::SecondaryAction(alt,false,false,true,2),"right-hand throw does not disable left alternate fire");
 require(!Controllers::SecondaryAction(alt,false,true,true,0),"hands-only mode does not fire a hidden tool");
 require(!Controllers::SecondaryAction(alt,true,false,true,0),"vehicle reverse trigger does not also send secondary fire");
 alt.grab=true;require(Controllers::SecondaryAction(alt,true,false,true,0),"vehicle secondary grip remains available");
 auto menuArea=HUDMath::Viewport(1920,1080,1,1.2f,0,0,0,true);auto playArea=HUDMath::Viewport(1920,1080,1,1.2f,0,0,0);
 require(menuArea[2]>playArea[2]&&menuArea[3]>playArea[3],"menu uses more eye pixels than gameplay HUD");
 require(std::abs(menuArea[1]+menuArea[3]/2-540)<=1,"menu image centered vertically");
 Controllers::BButtonGesture bg;
 require(bg.Update(true,true,100)==0&&bg.Update(false,true,300)==0&&bg.Update(false,true,599)==0&&bg.Update(false,true,600)==1,"single B waits for double-tap window before settings");
 require(bg.Update(true,true,700)==0&&bg.Update(true,true,1699)==0&&bg.Update(true,true,1700)==2,"hold B opens map after one second");
 require(bg.Update(true,true,2400)==0&&bg.Update(false,true,2500)==0,"map hold emits once and never opens settings on release");
 bg.Update(true,true,3000);require(bg.Update(false,true,4500)==2,"delayed B release still counts as map hold");
 bg.Update(true,true,5000);bg.Update(false,false,5100);require(bg.Update(true,true,7000)==0&&bg.Update(false,true,7100)==0,"tracking loss cancels B until release");
 require(bg.Update(true,true,7200)==0&&bg.Update(false,true,7250)==0&&bg.Update(false,true,7550)==1,"B recovers after cancelled gesture release");
 bg={};bg.Update(true,true,100);bg.Update(false,true,150);bg.Update(true,true,250);
 require(bg.Update(false,true,300)==3&&bg.Update(false,true,1000)==0,"double B pauses once without opening VR settings");
 bg={};bg.Update(true,true,100);bg.Update(false,true,150);bg.Update(true,true,250);
 require(bg.Update(true,true,1250)==2&&bg.Update(false,true,1300)==0,"holding second B press opens map without a pause or settings toggle");
 bg={};bg.Update(true,true,100);bg.Update(false,true,150);bg.Update(false,false,200);
 require(bg.Update(false,true,1000)==0,"focus loss cancels a pending single tap");
 auto panel=glm::translate(glm::mat4(1),glm::vec3(1,1.5f,-2))*glm::rotate(glm::mat4(1),.4f,glm::vec3(0,1,0));
 for(int row=0;row<MenuMath::Count;row++){
  auto cell=MenuMath::Cell(row);float u=float(cell.left+cell.right)*.5f/MenuMath::Width,v=float(cell.top+cell.bottom)*.5f/MenuMath::Height;
  auto target=glm::vec3(panel*glm::vec4((u-.5f)*1.6f,(.5f-v)*1.6f*MenuMath::Height/MenuMath::Width,0,1));
  for(float offset:{-.3f,.3f}){auto origin=glm::vec3(panel*glm::vec4(offset,-.3f,1.6f,1));auto controller=glm::inverse(glm::lookAt(origin,target,glm::vec3(0,1,0)));MenuMath::Hit hit;
   require(MenuMath::Intersect(panel,controller,1.6f,1.6f*MenuMath::Height/MenuMath::Width,hit)&&MenuMath::Row(hit.u,hit.v)==row&&glm::length(hit.world-target)<.0001f,"grid cursor and click share geometry from either hand");
  }
 }
 require(MenuMath::Row(.5f,.5f)==-1&&MenuMath::Row(.01f,.5f)==-1,"grid gutter and margins reject clicks");
 auto lightPush=ContactMath::PushVelocity(glm::vec3(0,0,-2),glm::vec3(0),glm::vec3(0,0,1),1.f);
 auto heavyPush=ContactMath::PushVelocity(glm::vec3(0,0,-2),glm::vec3(0),glm::vec3(0,0,1),.001f);
 require(glm::length(lightPush)>glm::length(heavyPush)*50,"heavy props respond less to hand pushes");
 require(glm::length(ContactMath::PushVelocity(glm::vec3(0,0,2),glm::vec3(0),glm::vec3(0,0,1),1))==0,"retreating hands do not pull objects");
 require(glm::length(ContactMath::PushVelocity(glm::vec3(0,0,-50),glm::vec3(0),glm::vec3(0,0,1),0))==0,"static bodies receive no velocity");
 MenuMath::Hit behind;require(!MenuMath::Intersect(glm::mat4(1),glm::translate(glm::mat4(1),glm::vec3(0,0,-1)),1,1,behind),"pointing away from menu cannot click it");
 auto seat=glm::translate(glm::mat4(1),glm::vec3(2,1.5f,4));auto seated=Controllers::AvatarCamera(seat,seat,{2,1.1f,4},true);
 require(seated[3]==glm::vec4(2,1.1f,4,1),"vehicle first-person camera follows seated avatar head height");
 auto orbit=glm::translate(seat,glm::vec3(0,1,3));require(Controllers::AvatarCamera(orbit,orbit,{2,1.1f,4},true)[3]==orbit[3],"vehicle third-person zoom is preserved");
 require(std::abs(Controllers::SeatHeight(-.4f)-.4f)<.001f&&Controllers::SeatHeight(-2)==.65f,"vehicle entry recenters sitting height with bounded correction");
 require(Controllers::CurrentHeadAnchor({0,1.7f,0},{0,1.8f,0},{.2f,1.8f,.1f})==glm::vec3(.2f,1.7f,.1f),"cached avatar head follows current locomotion translation");
 require(Controllers::CurrentHeadAnchor({0,1.7f,0},{0,1.8f,0},{10,2,0})==glm::vec3(10,2,0),"teleport discards stale avatar head offset");
 require(CameraAdjust::Step(.1f,.2f,.016f)==.1f,"camera adjustment deadzone prevents drift");
 require(CameraAdjust::Step(0,1,.02f)>0&&CameraAdjust::Step(0,-1,.02f)<0,"camera stick adjusts both directions");
 require(CameraAdjust::Step(.5f,1,.05f)==.5f&&CameraAdjust::Step(-.5f,-1,.05f)==-.5f,"camera offset stays within half metre bounds");
 require(CameraAdjust::Step(0,1,5.f)==CameraAdjust::Step(0,1,.05f),"stalled frame cannot jump camera adjustment");
 require(CameraAdjust::Step(.1f,std::numeric_limits<float>::quiet_NaN(),.01f)==.1f,"invalid stick sample cannot corrupt camera");
 float continuous=0;for(int i=0;i<50;i++)continuous=CameraAdjust::Step(continuous,1,.02f);
 require(std::abs(continuous-.15f)<.00001f,"camera adjustment runs at fifteen centimetres per second");
 {
 std::array<std::uint8_t,0x2450> renderer{};std::array<std::uint8_t,176> entries{};
 StereoSprites::Queue q{2,2,entries.data()};auto header=renderer.data()+0x2420;
 auto set=[&]{std::memcpy(header,&q,sizeof(q));};auto get=[&]{std::memcpy(&q,header,sizeof(q));};auto readable=[](const void* p,std::size_t){return p!=nullptr;};
 set();StereoSprites::Frame frame(renderer.data(),readable);frame.BeforeEye();get();require(q.count==2,"left eye retains native sprite queue");
 q.count=0;set();frame.BeforeEye();get();require(q.count==2,"right eye receives consumed laser sprites");
 q.count=0;set();require(*reinterpret_cast<int*>(header)==0,"final native consumption stays cleared for next frame");
 q.count=1;set();frame.BeforeEye();get();require(q.count==1,"new engine entries are never overwritten");
 q.count=0;q.data=nullptr;set();frame.BeforeEye();get();require(q.count==0,"changed sprite allocation is never revived");
 q={32769,32769,entries.data()};set();StereoSprites::Frame huge(renderer.data(),readable);q.count=0;set();huge.BeforeEye();get();require(q.count==0,"oversized sprite queue rejected without allocation");
 q={2,1,entries.data()};set();StereoSprites::Frame bad(renderer.data(),readable);q.count=0;set();bad.BeforeEye();get();require(q.count==0,"invalid sprite capacity rejected");
 }
 std::cout<<count<<" checks passed\n";return 0;
 }catch(std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
