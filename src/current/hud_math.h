#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <atomic>
namespace HUDMath {
inline std::atomic<bool> visible{true};
inline std::atomic<bool> interactive{false};
inline std::atomic<bool> tabletInteractive{false};
inline bool pointerVisible=false;inline float pointerU=.5f,pointerV=.5f;
inline std::array<int,4> Viewport(int width,int height,float p00,float p11,float p02,float p12,float eyeX,bool menu=false){
 // A shared plane 2.5 metres ahead puts both eye crosshairs at one point.
 // Keep the desktop HUD in the central field of view, including its bottom edge.
 float cx=.5f*(1.f-p02-p00*eyeX/2.5f),cy=.5f*(1.f-p12)-(menu?0.f:.035f);
 float halfWidth=menu?.95f:.75f,halfHeight=halfWidth*float(height)/float(width);
 float w=p00*halfWidth,h=p11*halfHeight;
 float fit=(std::min)({1.f,2.f*(std::min)(cx,1.f-cx)*.9f/w,2.f*(std::min)(cy,1.f-cy)*.9f/h});
 w*=fit;h*=fit;
 return {int(std::lround((cx-w*.5f)*width)),int(std::lround((cy-h*.5f)*height)),(std::max)(1,int(std::lround(w*width))),(std::max)(1,int(std::lround(h*height)))};
}
}
