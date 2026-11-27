#pragma once
#include <algorithm>
#include <cmath>
namespace CameraAdjust {
inline float Step(float position,float axis,float dt){
 if(!std::isfinite(axis)||!std::isfinite(dt)||dt<=0)return position;
 const float magnitude=(std::min)(1.f,std::abs(axis));
 if(magnitude<=.25f)return position;
 const float movement=std::copysign((magnitude-.25f)/.75f,axis)*.15f*(std::min)(dt,.05f);
 return (std::clamp)(position+movement,-.5f,.5f);
}
}
