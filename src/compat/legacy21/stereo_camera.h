#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <openvr.h>
#include <cmath>
#include <cstddef>
namespace Stereo {
// Recovered from 2.1.0 camera construction at +5EBCC0 and +5EC040.
struct Camera { glm::mat4 projection; glm::mat4 view; float nearClip; float farClip; };
static_assert(sizeof(Camera)==0x88);
static_assert(offsetof(Camera,view)==0x40);
static_assert(offsetof(Camera,nearClip)==0x80);
inline bool Valid(const Camera& c) {
 const float* p=reinterpret_cast<const float*>(&c);
 for(std::size_t i=0;i<sizeof(c)/sizeof(float);++i)if(!std::isfinite(p[i]))return false;
 return c.nearClip>0 && c.farClip>c.nearClip && std::abs(c.view[3][3]-1.f)<0.02f && std::abs(c.projection[2][3]+1.f)<0.02f && std::abs(glm::determinant(c.view))>0.01f;
}
inline glm::mat4 Pose(const vr::HmdMatrix34_t& m) {
 glm::mat4 r(1.f);for(int row=0;row<3;++row)for(int col=0;col<4;++col)r[col][row]=m.m[row][col];return r;
}
// Preserve position and heading, but keep the tracking space vertical.
// Inverting a tilted initial headset pose would bake its roll into the world.
inline glm::mat4 Upright(const glm::mat4& pose) {
 glm::vec3 back(pose[2].x,0.f,pose[2].z);
 if(glm::dot(back,back)<0.000001f) {
  glm::vec3 right(pose[0].x,0.f,pose[0].z);
  back=glm::cross(glm::normalize(right),glm::vec3(0,1,0));
 }
 back=glm::normalize(back);
 glm::mat4 result(1.f);
 result[0]=glm::vec4(glm::cross(glm::vec3(0,1,0),back),0);
 result[1]=glm::vec4(0,1,0,0);
 result[2]=glm::vec4(back,0);
 result[3]=pose[3];
 return result;
}
inline Camera EyeCamera(const Camera& game,const glm::mat4& relativeHead,const glm::mat4& eyeToHead,const vr::HmdMatrix44_t& projection) {
 Camera eye=game;
 eye.view=glm::inverse(Upright(glm::inverse(game.view))*relativeHead*eyeToHead);
 // Keep the game's depth convention (OpenGL versus 0..1 clip depth); replace only the eye frustum.
 for(int col=0;col<4;++col)for(int row=0;row<2;++row)eye.projection[col][row]=projection.m[row][col];
 return eye;
}
}
