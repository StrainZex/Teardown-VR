#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <GL/glew.h>
#include <detours.h>
#include <openvr.h>
#include <atomic>
#include <cstdint>
#include <string>
#include <iostream>
#include <stdexcept>
struct EyeTarget{GLuint texture=0,fbo=0;int width=32,height=32;} targets[2];
std::atomic<bool> enabled{true};unsigned long long frames=1;
vr::IVRCompositor* compositor=nullptr;
void Log(const std::string& s){std::cout<<s<<'\n';}
void Phase(const char*){}
#include "inline_hud.h"
int main(){
 HWND window=CreateWindowExW(0,L"STATIC",L"Sledge HUD test",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR format{sizeof(format),1,PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,PFD_TYPE_RGBA,32};
 int pixel=ChoosePixelFormat(dc,&format);SetPixelFormat(dc,pixel,&format);HGLRC context=wglCreateContext(dc);
 int result=0;
 try{
  if(!context||!wglMakeCurrent(dc,context)||glewInit()!=GLEW_OK||!GLEW_VERSION_3_0)throw std::runtime_error("OpenGL test context unavailable");
  for(int i=0;i<2;i++){auto& t=targets[i];glGenTextures(1,&t.texture);glBindTexture(GL_TEXTURE_2D,t.texture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,32,32,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glGenFramebuffers(1,&t.fbo);glBindFramebuffer(GL_FRAMEBUFFER,t.fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,t.texture,0);glClearColor(0,i==1?1.f:0.f,i==0?1.f:0.f,1);glClear(GL_COLOR_BUFFER_BIT);}
  glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,64,64);glScissor(16,16,32,32);glEnable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  InlineHUD::Begin(false);
  InlineHUD::Replay([]{glColor4f(1,0,0,.5f);glBegin(GL_TRIANGLES);glVertex2f(-1,-1);glVertex2f(3,-1);glVertex2f(-1,3);glEnd();});
  GLint fbo,viewport[4],scissor[4];glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&fbo);glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_SCISSOR_BOX,scissor);
  if(fbo!=0||viewport[2]!=64||scissor[0]!=16||scissor[2]!=32)throw std::runtime_error("UI replay did not restore framebuffer/viewport/scissor");
  glEnable(GL_DEPTH_TEST);bool wronglyReplayed=false;InlineHUD::Replay([&]{wronglyReplayed=true;});glDisable(GL_DEPTH_TEST);
  if(wronglyReplayed||InlineHUD::rejectedDepth!=1)throw std::runtime_error("world draw was incorrectly replayed as HUD");
  HUDMath::visible=false;bool hiddenDraw=false;InlineHUD::Replay([&]{hiddenDraw=true;});HUDMath::visible=true;
  if(hiddenDraw)throw std::runtime_error("disabled HUD still draws into VR eyes");
  HUDMath::tabletInteractive=true;bool leakedMenu=false;InlineHUD::Replay([&]{leakedMenu=true;});HUDMath::tabletInteractive=false;
  if(leakedMenu)throw std::runtime_error("tablet menu leaked into eye images");
  // Exercise the exact base-vertex callback used by modern UI renderers.
  InlineHUD::baseVertex=glDrawElementsBaseVertex;
  InlineHUD::BaseVertex(GL_TRIANGLES,0,GL_UNSIGNED_SHORT,nullptr,0);
  if(InlineHUD::draws!=2)throw std::runtime_error("base-vertex callback bypassed eye replay");
  for(int i=0;i<2;i++){
   unsigned char color[4]{};glBindFramebuffer(GL_READ_FRAMEBUFFER,targets[i].fbo);glReadBuffer(GL_COLOR_ATTACHMENT0);glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,color);
   if(color[0]<126||color[0]>130||color[i==0?2:1]<126||color[i==0?2:1]>130||color[i==0?1:2]!=0)throw std::runtime_error("UI alpha or independent eye background not preserved");
   glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,color);if(color[0]!=0||color[i==0?2:1]!=255)throw std::runtime_error("scaled UI clip rectangle not preserved");
  }
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);HUDMath::interactive=true;HUDMath::pointerVisible=true;HUDMath::pointerU=HUDMath::pointerV=.5f;glClearColor(.2f,.3f,.4f,.5f);glColorMask(GL_TRUE,GL_FALSE,GL_TRUE,GL_FALSE);
  InlineHUD::DrawPointer();GLfloat clear[4]{};GLboolean mask[4]{};glGetFloatv(GL_COLOR_CLEAR_VALUE,clear);glGetBooleanv(GL_COLOR_WRITEMASK,mask);glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&fbo);glGetIntegerv(GL_SCISSOR_BOX,scissor);
  if(fbo!=0||scissor[0]!=16||scissor[2]!=32||mask[1]!=GL_FALSE||mask[3]!=GL_FALSE||std::abs(clear[0]-.2f)>.001f)throw std::runtime_error("controller pointer leaked GL state");
  for(int i=0;i<2;i++){unsigned char color[4]{};glBindFramebuffer(GL_READ_FRAMEBUFFER,targets[i].fbo);glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,color);if(color[0]!=255||color[1]!=255||color[2]!=255)throw std::runtime_error("controller pointer missing from eye");}
  std::cout<<"PASS live OpenGL: both-eye UI blend, clipping, visible controller cursor and state restoration\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';result=1;}
 wglMakeCurrent(nullptr,nullptr);if(context)wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);return result;
}
