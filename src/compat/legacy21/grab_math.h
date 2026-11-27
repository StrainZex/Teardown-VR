#pragma once
#include "controller_math.h"
namespace GrabMath {
struct TriggerThrow {
 bool charging=false;std::uint64_t start=0;
 float Update(bool trigger,bool holding,std::uint64_t now){
  if(!holding){charging=false;return 0;}
  if(trigger&&!charging){charging=true;start=now;}
  if(!trigger&&charging){charging=false;return 3.f+9.f*glm::clamp(float(now-start)/1000.f,0.f,1.f);}
  return 0;
 }
};
inline glm::vec3 Limit(glm::vec3 v,float maximum){float n=glm::length(v);return n>maximum?v*(maximum/n):v;}
inline glm::vec3 FollowVelocity(glm::vec3 anchor,glm::vec3 target,glm::vec3 handVelocity){return Limit(handVelocity+(target-anchor)*18.f,12.f);}
inline glm::vec3 AngularVelocity(const glm::mat4& current,const glm::mat4& target){auto q=glm::normalize(glm::quat_cast(glm::mat3(target))*glm::inverse(glm::quat_cast(glm::mat3(current))));if(q.w<0)q=-q;float n=glm::length(glm::vec3(q.x,q.y,q.z));return n<.00001f?glm::vec3(0):Limit(glm::vec3(q.x,q.y,q.z)/n*(2.f*std::atan2(n,q.w)*12.f),12.f);}
}
