#pragma once
#include "hud_math.h"
// Replay late blended, depth-free UI draws into both existing eye targets.
// This keeps the native UI shader, textures, colors and blending intact and
// avoids trying to extract transparency from a screenshot of the game world.
namespace InlineHUD {
using ElementsFn=void(APIENTRY*)(GLenum,GLsizei,GLenum,const void*);
using ArraysFn=void(APIENTRY*)(GLenum,GLint,GLsizei);
ElementsFn elements=nullptr;ArraysFn arrays=nullptr;
PFNGLDRAWELEMENTSBASEVERTEXPROC baseVertex=nullptr;
PFNGLDRAWELEMENTSINSTANCEDPROC instanced=nullptr;
PFNGLDRAWARRAYSINSTANCEDPROC arraysInstanced=nullptr;
bool pending=false,onePass=false;GLint sceneFbo=0;HGLRC context=nullptr;
unsigned draws=0;bool reported=false;
unsigned candidates=0,rejectedFbo=0,rejectedDepth=0,rejectedBlend=0,rejectedStencil=0;
std::array<int,4> eyeViewport[2]{};
thread_local bool insideDraw=false;
template<class Draw> void Replay(Draw draw);
template<class Draw> void Dispatch(Draw draw){if(insideDraw){draw();return;}insideDraw=true;draw();Replay(draw);insideDraw=false;}
template<class Draw> void Replay(Draw draw){
 if(HUDMath::tabletInteractive)return;
 if((!HUDMath::visible&&!HUDMath::interactive)||!pending||!enabled||wglGetCurrentContext()!=context)return;
 ++candidates;
 GLint fbo=0;glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&fbo);
 if(fbo!=0&&fbo!=sceneFbo){++rejectedFbo;return;}
 if(glIsEnabled(GL_DEPTH_TEST)){++rejectedDepth;return;}
 if(!glIsEnabled(GL_BLEND)){++rejectedBlend;return;}
 if(glIsEnabled(GL_STENCIL_TEST)){++rejectedStencil;return;}
 GLint viewport[4]{},scissor[4]{};glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_SCISSOR_BOX,scissor);
 if(viewport[2]<=0||viewport[3]<=0)return;
 // Replay only on full-size scene/UI surfaces, never thumbnail viewports.
 if(viewport[0]!=0||viewport[1]!=0)return;
 for(int eye=0;eye<(onePass?1:2);eye++){
  auto& t=targets[eye];if(!t.fbo)continue;
  auto area=eyeViewport[eye];if(!area[2])area={0,0,t.width,t.height};
  float sx=float(area[2])/viewport[2],sy=float(area[3])/viewport[3];
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER,t.fbo);glViewport(area[0],area[1],area[2],area[3]);
  glScissor(area[0]+int(scissor[0]*sx),area[1]+int(scissor[1]*sy),int(scissor[2]*sx),int(scissor[3]*sy));draw();
 }
 glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fbo);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glScissor(scissor[0],scissor[1],scissor[2],scissor[3]);++draws;
}
void APIENTRY Elements(GLenum mode,GLsizei count,GLenum type,const void* indices){Dispatch([&]{elements(mode,count,type,indices);});}
void APIENTRY Arrays(GLenum mode,GLint first,GLsizei count){Dispatch([&]{arrays(mode,first,count);});}
void APIENTRY BaseVertex(GLenum mode,GLsizei count,GLenum type,const void* indices,GLint base){Dispatch([&]{baseVertex(mode,count,type,indices,base);});}
void APIENTRY Instanced(GLenum mode,GLsizei count,GLenum type,const void* indices,GLsizei instances){Dispatch([&]{instanced(mode,count,type,indices,instances);});}
void APIENTRY ArraysInstanced(GLenum mode,GLint first,GLsizei count,GLsizei instances){Dispatch([&]{arraysInstanced(mode,first,count,instances);});}
void Begin(bool mono){onePass=mono;glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&sceneFbo);context=wglGetCurrentContext();draws=candidates=rejectedFbo=rejectedDepth=rejectedBlend=rejectedStencil=0;pending=true;}
void DrawPointer(){
 if(!HUDMath::interactive||!HUDMath::pointerVisible)return;
 GLint fbo=0,scissor[4]{};GLfloat color[4]{};GLboolean mask[4]{};bool scissorEnabled=glIsEnabled(GL_SCISSOR_TEST)!=0;
 glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&fbo);glGetIntegerv(GL_SCISSOR_BOX,scissor);glGetFloatv(GL_COLOR_CLEAR_VALUE,color);glGetBooleanv(GL_COLOR_WRITEMASK,mask);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glEnable(GL_SCISSOR_TEST);
 for(int eye=0;eye<(onePass?1:2);eye++){
  auto& t=targets[eye];auto area=eyeViewport[eye];if(!area[2])area={0,0,t.width,t.height};int x=area[0]+int(HUDMath::pointerU*area[2]),y=area[1]+int((1-HUDMath::pointerV)*area[3]);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER,t.fbo);int radius=(std::max)(5,area[2]/120);
  glScissor(x-radius-1,y-radius-1,radius*2+3,radius*2+3);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
  glScissor(x-radius,y-radius,radius*2+1,radius*2+1);glClearColor(1,1,1,1);glClear(GL_COLOR_BUFFER_BIT);
  glScissor(x-1,y-radius*2,3,radius*4+1);glClear(GL_COLOR_BUFFER_BIT);glScissor(x-radius*2,y-1,radius*4+1,3);glClear(GL_COLOR_BUFFER_BIT);
 }
 glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fbo);glScissor(scissor[0],scissor[1],scissor[2],scissor[3]);glClearColor(color[0],color[1],color[2],color[3]);glColorMask(mask[0],mask[1],mask[2],mask[3]);if(!scissorEnabled)glDisable(GL_SCISSOR_TEST);
}
void Submit(){
 if(!pending||wglGetCurrentContext()!=context)return;pending=false;
 if(!enabled)return;
 DrawPointer();auto submitStart=GetTickCount64();glFlush();
 for(int eye=0;eye<2;eye++){vr::Texture_t texture{reinterpret_cast<void*>(static_cast<std::uintptr_t>(targets[onePass?0:eye].texture)),vr::TextureType_OpenGL,vr::ColorSpace_Auto};auto error=compositor->Submit(static_cast<vr::EVREye>(eye),&texture);if(error!=vr::VRCompositorError_None&&frames%120==0)Log("Submit error "+std::to_string(error));}
 if(GetTickCount64()-submitStart>250)Log("Slow VR submit ms="+std::to_string(GetTickCount64()-submitStart));
 if(!reported||frames%300==0){Log("Inline HUD replay draws="+std::to_string(draws)+" candidates="+std::to_string(candidates)+" filtered fbo/depth/blend/stencil="+std::to_string(rejectedFbo)+"/"+std::to_string(rejectedDepth)+"/"+std::to_string(rejectedBlend)+"/"+std::to_string(rejectedStencil));reported=true;}
}
LONG Attach(std::uint8_t* game){
 auto module=GetModuleHandleW(L"opengl32.dll");elements=reinterpret_cast<ElementsFn>(GetProcAddress(module,"glDrawElements"));arrays=reinterpret_cast<ArraysFn>(GetProcAddress(module,"glDrawArrays"));
 if(!elements||!arrays)return ERROR_PROC_NOT_FOUND;
 auto error=DetourAttach(reinterpret_cast<PVOID*>(&elements),reinterpret_cast<PVOID>(&Elements));
 if(error==NO_ERROR)error=DetourAttach(reinterpret_cast<PVOID*>(&arrays),reinterpret_cast<PVOID>(&Arrays));
 // Startup waits for these loader pointers before beginning the all-thread
 // Detours transaction, including when the launcher injects before a level.
 baseVertex=*reinterpret_cast<PFNGLDRAWELEMENTSBASEVERTEXPROC*>(game+0x1FC2838);
 instanced=*reinterpret_cast<PFNGLDRAWELEMENTSINSTANCEDPROC*>(game+0x1FC7878);
 arraysInstanced=*reinterpret_cast<PFNGLDRAWARRAYSINSTANCEDPROC*>(game+0x1FC7870);
 if(error==NO_ERROR&&baseVertex)error=DetourAttach(reinterpret_cast<PVOID*>(&baseVertex),reinterpret_cast<PVOID>(&BaseVertex));
 if(error==NO_ERROR&&instanced)error=DetourAttach(reinterpret_cast<PVOID*>(&instanced),reinterpret_cast<PVOID>(&Instanced));
 if(error==NO_ERROR&&arraysInstanced)error=DetourAttach(reinterpret_cast<PVOID*>(&arraysInstanced),reinterpret_cast<PVOID>(&ArraysInstanced));
 Log("HUD modern draw hooks: baseVertex="+std::to_string(baseVertex!=nullptr)+" elementsInstanced="+std::to_string(instanced!=nullptr)+" arraysInstanced="+std::to_string(arraysInstanced!=nullptr));return error;
}
}
