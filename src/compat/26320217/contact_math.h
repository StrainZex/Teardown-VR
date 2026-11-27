#pragma once
#include "grab_math.h"
namespace ContactMath {
inline glm::vec3 PushVelocity(glm::vec3 motion,glm::vec3 velocity,glm::vec3 normal,float inverseMass){
 if(!std::isfinite(inverseMass)||inverseMass<=0)return glm::vec3(0);
 float closing=glm::clamp(-glm::dot(motion-velocity,normal),0.f,6.f);
 // A small effective hand mass; heavy objects respond less than light props.
 float transfer=2.f*inverseMass/(1.f+2.f*inverseMass);
 return -normal*(std::min)(.35f,closing*transfer*.25f);
}
inline void InsetBounds(glm::vec3& low,glm::vec3& high){
 auto inset=glm::min((high-low)*.2f,glm::vec3(.015f));low+=inset;high-=inset;
}
inline glm::vec3 GentlePush(glm::vec3 motion,glm::vec3 velocity,glm::vec3 normal,float pressure=0){
 motion=GrabMath::Limit(motion,1.2f)-normal*glm::clamp(pressure*4.f,0.f,.5f);float inward=(std::max)(0.f,-glm::dot(motion-velocity,normal));
 return -normal*(std::min)(.08f,inward*.25f);
}

inline glm::mat4 TwoHand(const glm::mat4& tool,glm::vec3 primary,glm::vec3 support,glm::vec3 primaryWorld,glm::vec3 supportWorld){
 auto a=glm::mat3(tool)*(support-primary),b=supportWorld-primaryWorld;
 if(glm::length(a)<.04f||glm::length(b)<.04f)return tool;
 a=glm::normalize(a);b=glm::normalize(b);float d=glm::clamp(glm::dot(a,b),-1.f,1.f);
 glm::quat q;
 if(d<-.9999f){auto axis=glm::cross(a,glm::vec3(0,1,0));if(glm::length(axis)<.01f)axis=glm::cross(a,glm::vec3(1,0,0));q=glm::angleAxis(glm::pi<float>(),glm::normalize(axis));}
 else{auto cross=glm::cross(a,b);q=glm::normalize(glm::quat(1.f+d,cross.x,cross.y,cross.z));}
 auto out=glm::mat4(glm::mat3_cast(q)*glm::mat3(tool));out[3]=glm::vec4(primaryWorld-glm::mat3(out)*primary,1);return out;
}
// Swept sphere versus expanded bounds. Starting inside is ignored to allow
// recovery after loading/recentering; this is a coarse contact approximation.
inline bool Sweep(glm::vec3 from,glm::vec3 to,glm::vec3 low,glm::vec3 high,float radius,float& time,glm::vec3& normal){
 low-=radius;high+=radius;auto delta=to-from;float enter=0,leave=1;glm::vec3 n(0);bool inside=true;
 for(int i=0;i<3;i++)if(from[i]<low[i]||from[i]>high[i])inside=false;
 if(inside)return false;
 for(int i=0;i<3;i++){
  if(std::abs(delta[i])<.000001f){if(from[i]<low[i]||from[i]>high[i])return false;continue;}
  float a=(low[i]-from[i])/delta[i],b=(high[i]-from[i])/delta[i];float sign=-1;
  if(a>b){std::swap(a,b);sign=1;}
  if(a>enter){enter=a;n=glm::vec3(0);n[i]=sign;}leave=(std::min)(leave,b);if(enter>leave)return false;
 }
 if(enter<0||enter>1||glm::length(n)<.5f)return false;time=enter;normal=n;return true;
}
}
