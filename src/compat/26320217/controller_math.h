#pragma once
#include "stereo_camera.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace Controllers {
struct BButtonGesture {
 bool held=false,fired=false,blocked=false,pending=false,second=false;unsigned long long start=0,deadline=0;
 // Single tap waits 300 ms for a second tap: 1=settings, 2=hold/map, 3=pause.
 int Update(bool down,bool available,unsigned long long now){
  if(!available){held=false;fired=false;pending=false;second=false;blocked=true;return 0;}
  if(blocked){if(!down)blocked=false;return 0;}
  int action=0;if(pending&&!held&&now>=deadline){pending=false;action=1;}
  if(down&&!held){held=true;fired=false;second=pending;start=now;}
  if(held&&!fired&&now-start>=1000){fired=true;pending=false;second=false;if(!down)held=false;return 2;}
  if(!down&&held){held=false;if(fired)return action;if(second){pending=false;second=false;return 3;}pending=true;deadline=now+300;}
  return action;
 }
};
struct WheelRepeat {
 int previous=0;unsigned long long next=0;
 bool Update(int value,bool vehicle,unsigned long long now){
  bool emit=value&&(value!=previous||(vehicle&&now>=next));previous=value;
  if(emit)next=now+100;if(!value)next=0;return emit;
 }
};
struct Transform { float x,y,z,qx,qy,qz,qw; };
struct HandPose { Transform transform; bool used; char pad[3]{}; };
static_assert(sizeof(Transform)==28 && sizeof(HandPose)==32);
inline bool Valid(const Transform& t){
 const float* f=&t.x;for(int i=0;i<7;i++)if(!std::isfinite(f[i]))return false;
 float norm=t.qx*t.qx+t.qy*t.qy+t.qz*t.qz+t.qw*t.qw;return norm>.8f&&norm<1.2f;
}
inline glm::mat4 Matrix(const Transform& t){
 auto m=glm::mat4_cast(glm::normalize(glm::quat(t.qw,t.qx,t.qy,t.qz)));m[3]=glm::vec4(t.x,t.y,t.z,1);return m;
}
inline Transform Pack(const glm::mat4& m){auto q=glm::normalize(glm::quat_cast(glm::mat3(m)));return {m[3].x,m[3].y,m[3].z,q.x,q.y,q.z,q.w};}
inline glm::mat4 AvatarCamera(glm::mat4 base,const glm::mat4& camera,glm::vec3 head,bool vehicle){
 // Only replace a nearby first-person camera. Keep native vehicle zoom and
 // third-person orbit when the camera is outside the driver's head region.
 if(glm::length(head-glm::vec3(camera[3]))<(vehicle?.8f:1.5f))base[3]=glm::vec4(head,1);
 return base;
}
inline glm::vec3 CurrentHeadAnchor(glm::vec3 capturedHead,glm::vec3 capturedCamera,glm::vec3 currentCamera){
 auto movement=currentCamera-capturedCamera;
 return glm::length(movement)<.75f?capturedHead+movement:currentCamera;
}
inline float SeatHeight(float relativeHeadY){return glm::clamp(-relativeHeadY,-.65f,.65f);}
inline glm::mat4 ToolAtGrip(const glm::mat4& hand,const Transform& grip){return hand*glm::inverse(Matrix(grip));}
// The authored tool rotation contains model-axis corrections. A hand bone's
// quaternion is not an aiming orientation and must not replace that rotation.
inline glm::mat4 AuthoredToolAtHand(const glm::mat4& hand,const glm::mat4& camera,const Transform& authored,const HandPose& grip){
 auto rotation=glm::mat4(glm::mat3(hand)*glm::transpose(glm::mat3(camera))*glm::mat3(Matrix(authored)));
 glm::vec3 anchor(0.f);
 if(grip.used&&Valid(grip.transform)){
  glm::vec3 candidate(grip.transform.x,grip.transform.y,grip.transform.z);
  if(glm::length(candidate)<=1.f)anchor=candidate;
 }
 rotation[3]=glm::vec4(glm::vec3(hand[3])-glm::mat3(rotation)*anchor,1.f);
 return rotation;
}
inline float NextTurnSpeed(float speed){for(float next:{45.f,90.f,150.f,180.f,240.f,360.f})if(next>speed+.5f)return next;return 45.f;}
// Native tool IK targets use +90 degrees about Y (gun/rifle/rocket prefabs).
// Apply this only to the animation target, never to the weapon aiming frame.
inline glm::mat4 AnimationHand(const glm::mat4& controller){return glm::rotate(controller,glm::radians(90.f),glm::vec3(0,1,0));}
inline bool UsableGrip(const HandPose& grip){return grip.used&&Valid(grip.transform)&&glm::length(glm::vec3(grip.transform.x,grip.transform.y,grip.transform.z))<=1.f;}
inline glm::vec3 Aim(const glm::mat4& hand){return glm::normalize(-glm::vec3(hand[2]));}
struct XAction {bool reload=false,interact=false;};
// mode: 0 = no prompt/reload, 1 = ordinary interaction/vehicle exit,
// 2 = vehicle entry. A changed target cancels a held gesture until release.
struct XButtonGesture {
 bool held=false,longPress=false,cancelled=false;int latchedMode=0;std::uint64_t start=0,target=0;
 XAction Update(bool down,bool active,std::uint64_t now,int mode=2,std::uint64_t identity=0){
  XAction action;if(!active){held=false;longPress=false;cancelled=false;return action;}
  if(down&&!held){start=now;longPress=false;cancelled=false;latchedMode=mode;target=identity;if(mode==1){action.interact=true;longPress=true;}}
  if(held&&(mode!=latchedMode||identity!=target))cancelled=true;
  if(down&&!cancelled&&!longPress&&latchedMode==2&&now-start>=1000){action.interact=true;longPress=true;}
  if(!down&&held&&!cancelled&&!longPress){if(latchedMode==2&&now-start>=1000)action.interact=true;else if(latchedMode==0)action.reload=true;}
  held=down;return action;
 }
};
struct Buttons { bool forward=false,back=false,left=false,right=false,jump=false,crouch=false,interact=false,light=false,fire=false,leftTrigger=false,grab=false,leftGrab=false,sprint=false,menu=false; float turn=0;int wheel=0; };
inline int MenuScroll(float left,float right){float value=std::abs(left)>std::abs(right)?left:right;return value>.35f?1:value<-.35f?-1:0;}
inline bool SecondaryAction(const Buttons& b,bool driving,bool handsOnly,bool separate,unsigned throwMask){
 if(driving)return b.grab;
 return (!handsOnly&&b.leftTrigger&&!(throwMask&1u))||(!separate&&(b.leftGrab||b.grab));
}
inline Buttons VehicleButtons(Buttons b){
 bool accelerate=b.fire,reverse=b.leftTrigger,primary=b.grab,secondary=b.leftGrab;
 b.forward=accelerate;b.back=reverse;b.fire=primary;b.grab=secondary;b.leftGrab=false;
 b.crouch=false;b.sprint=false;return b;
}
inline Buttons Map(const vr::VRControllerState_t& left,const vr::VRControllerState_t& right){
 auto down=[](const vr::VRControllerState_t& s,vr::EVRButtonId id){return (s.ulButtonPressed&vr::ButtonMaskFromId(id))!=0;};
 Buttons b;b.forward=left.rAxis[0].y>.35f;b.back=left.rAxis[0].y<-.35f;b.left=left.rAxis[0].x<-.35f;b.right=left.rAxis[0].x>.35f;
 b.turn=std::abs(right.rAxis[0].x)>.25f?right.rAxis[0].x:0;
 b.wheel=right.rAxis[0].y>.7f?1:right.rAxis[0].y<-.7f?-1:0;
 b.jump=down(right,vr::k_EButton_A);b.menu=down(right,vr::k_EButton_ApplicationMenu);
 bool lc=down(left,vr::k_EButton_SteamVR_Touchpad),rc=down(right,vr::k_EButton_SteamVR_Touchpad);
 b.sprint=lc&&!rc;b.crouch=rc&&!lc;
 b.interact=down(left,vr::k_EButton_A);b.light=down(left,vr::k_EButton_ApplicationMenu);
 b.leftTrigger=left.rAxis[1].x>.55f||down(left,vr::k_EButton_SteamVR_Trigger);
 b.fire=right.rAxis[1].x>.55f||down(right,vr::k_EButton_SteamVR_Trigger);
 b.grab=right.rAxis[2].x>.55f||down(right,vr::k_EButton_Grip)||down(right,vr::k_EButton_Axis2);
 b.leftGrab=left.rAxis[2].x>.55f||down(left,vr::k_EButton_Grip)||down(left,vr::k_EButton_Axis2);
 return b;
}
}
