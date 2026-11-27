#pragma once
#include "stereo_camera.h"
namespace MenuMath {
struct Hit {float u=0,v=0;glm::vec3 world{};};
inline bool Intersect(const glm::mat4& panel,const glm::mat4& controller,float width,float height,Hit& hit){
 auto local=glm::inverse(panel)*controller;glm::vec3 origin(local[3]),direction=-glm::vec3(local[2]);
 if(direction.z>=-.00001f)return false;float distance=-origin.z/direction.z;if(distance<0)return false;
 auto p=origin+direction*distance;hit.u=.5f+p.x/width;hit.v=.5f-p.y/height;hit.world=glm::vec3(panel*glm::vec4(p.x,p.y,0,1));
 return hit.u>=0&&hit.u<=1&&hit.v>=0&&hit.v<=1;
}
struct Rect {int left,top,right,bottom;};
inline constexpr int Width=1280,Height=1416,Count=23;
inline Rect Cell(int index){int x=28+(index%2)*622,y=112+(index/2)*104;return {x,y,x+602,y+88};}
inline int Row(float u,float v,int width=Width,int height=Height){
 float x=u*width,y=v*height;
 for(int i=0;i<Count;i++){auto r=Cell(i);if(x>=r.left&&x<r.right&&y>=r.top&&y<r.bottom)return i;}
 return -1;
}
}
