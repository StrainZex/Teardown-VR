#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <intrin.h>
#include <TlHelp32.h>
#include <GL/glew.h>
#include <detours.h>
#include <openvr.h>
#include <atomic>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <vector>
#include <deque>
#include <array>
#include <string>
#include <algorithm>
#include "current_build.h"
#include "stereo_camera.h"
#include "controller_math.h"
#include "grab_math.h"
#include "contact_math.h"
#include "hud_math.h"
namespace {
using RenderFn=void(*)(void*,void*,void*,Stereo::Camera*);
RenderFn original=nullptr;
HMODULE self=nullptr;
std::atomic<bool> enabled{false};
std::atomic<bool> ready{false};
std::ofstream logFile;
std::mutex logMutex;
std::deque<std::string> logQueue;
std::atomic<const char*> renderPhase{"between render callbacks"};std::atomic<ULONGLONG> phaseStart{0};
void Phase(const char* name){renderPhase=name;phaseStart=GetTickCount64();}
vr::IVRSystem* systemVR=nullptr;
vr::IVRCompositor* compositor=nullptr;
bool vrAttempted=false, glReady=false, centerSet=false;
unsigned centerSerial=0;
glm::mat4 centerInverse(1.f);
struct EyeTarget { GLuint texture=0,fbo=0; int width=0,height=0; };
EyeTarget targets[2];
unsigned long long frames=0;
void Log(const std::string& text) {std::unique_lock<std::mutex> lock(logMutex,std::try_to_lock);if(lock&&logQueue.size()<256)logQueue.push_back(std::to_string(GetTickCount64())+" "+text);}
DWORD WINAPI LogWorker(void*){
 ULONGLONG reported=0;
 for(;;){Sleep(100);std::deque<std::string> pending;{std::lock_guard<std::mutex> lock(logMutex);pending.swap(logQueue);}
  for(auto& line:pending)logFile<<line<<'\n';if(!pending.empty())logFile.flush();
  auto start=phaseStart.load();if(start&&start!=reported&&GetTickCount64()-start>1500){reported=start;Log(std::string("Stall watchdog: ")+renderPhase.load()+" elapsed ms="+std::to_string(GetTickCount64()-start));}
 }
}
bool Readable(const void* p,std::size_t bytes) {
 auto at=reinterpret_cast<std::uintptr_t>(p);if(!at || at+bytes<at)return false;
 while(bytes){MEMORY_BASIC_INFORMATION m{};if(!VirtualQuery(reinterpret_cast<void*>(at),&m,sizeof(m))||m.State!=MEM_COMMIT||(m.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return false;
 auto available=reinterpret_cast<std::uintptr_t>(m.BaseAddress)+m.RegionSize-at;auto step=(bytes<available?bytes:available);if(!step)return false;at+=step;bytes-=step;}return true;
}
bool InitVR() {
 if(systemVR&&compositor)return true;
 static ULONGLONG retryAt=0;
 if(vrAttempted&&GetTickCount64()<retryAt)return false;
 vrAttempted=true;retryAt=GetTickCount64()+2000;
 if(!vr::VR_IsRuntimeInstalled()||!vr::VR_IsHmdPresent()){Log("Waiting for SteamVR/headset; retrying automatically.");return false;}
 vr::EVRInitError e=vr::VRInitError_None;systemVR=vr::VR_Init(&e,vr::VRApplication_Scene);
 if(e!=vr::VRInitError_None||!systemVR){Log(std::string("OpenVR init failed: ")+vr::VR_GetVRInitErrorAsEnglishDescription(e));return false;}
 compositor=vr::VRCompositor();if(!compositor){Log("OpenVR compositor unavailable; retrying");vr::VR_Shutdown();systemVR=nullptr;return false;}
 compositor->SetTrackingSpace(vr::TrackingUniverseStanding);Log("OpenVR initialized; experimental stereo enabled");return true;
}
#include "controller_bridge.h"
#include "tablet_ui.h"
#include "game_pointer.h"
#include "vr_menu.h"
void CheckVRRecenter() {
 vr::VREvent_t event{};
 for(int i=0;i<64&&systemVR->PollNextEvent(&event,sizeof(event));++i){
  if(event.eventType==vr::VREvent_SeatedZeroPoseReset||event.eventType==vr::VREvent_StandingZeroPoseReset||event.eventType==vr::VREvent_ChaperoneUniverseHasChanged){
   centerSet=false;Log("SteamVR tracking-origin reset received: "+std::to_string(event.eventType));
  }
 }
 // Legacy OpenVR maps joystick clicks to the axis-0 (touchpad) press bit.
 const auto mask=vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad);
 auto pressed=[&](vr::ETrackedControllerRole role){
  auto index=systemVR->GetTrackedDeviceIndexForControllerRole(role);
  vr::VRControllerState_t state{};
  return index!=vr::k_unTrackedDeviceIndexInvalid&&systemVR->GetControllerState(index,&state,sizeof(state))&&(state.ulButtonPressed&mask)!=0;
 };
 bool held=pressed(vr::TrackedControllerRole_LeftHand)&&pressed(vr::TrackedControllerRole_RightHand);
 static ULONGLONG holdStart=0;static bool latched=false;
 if(!held){holdStart=0;latched=false;return;}
 auto now=GetTickCount64();if(!holdStart)holdStart=now;
 if(!latched&&now-holdStart>=1500){centerSet=false;latched=true;Log("Recenter requested by holding both controller sticks");}
}
bool Capture(EyeTarget& eye) {
 GLint viewport[4]{},draw=0,read=0,texture=0,readBuffer=0;
 glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);glGetIntegerv(GL_READ_BUFFER,&readBuffer);
 const int w=viewport[2],h=viewport[3];if(w<1||h<1||w>16384||h>16384)return false;
 bool scissor=glIsEnabled(GL_SCISSOR_TEST)==GL_TRUE;glDisable(GL_SCISSOR_TEST);
 bool srgb=glIsEnabled(GL_FRAMEBUFFER_SRGB)==GL_TRUE;glDisable(GL_FRAMEBUFFER_SRGB);
 bool ok=true;
 if(!eye.fbo||eye.width!=w||eye.height!=h){
  if(eye.fbo)glDeleteFramebuffers(1,&eye.fbo);if(eye.texture)glDeleteTextures(1,&eye.texture);
  glGenTextures(1,&eye.texture);glBindTexture(GL_TEXTURE_2D,eye.texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
  glGenFramebuffers(1,&eye.fbo);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,eye.fbo);glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,eye.texture,0);eye.width=w;eye.height=h;
 }
 glBindFramebuffer(GL_DRAW_FRAMEBUFFER,eye.fbo);glDrawBuffer(GL_COLOR_ATTACHMENT0);
 glBindFramebuffer(GL_READ_FRAMEBUFFER,draw);GLint sourceRead=0;glGetIntegerv(GL_READ_BUFFER,&sourceRead);glReadBuffer(draw?GL_COLOR_ATTACHMENT0:GL_BACK);
 if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE||glCheckFramebufferStatus(GL_READ_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)ok=false;
 if(ok)glBlitFramebuffer(viewport[0],viewport[1],viewport[0]+w,viewport[1]+h,0,0,w,h,GL_COLOR_BUFFER_BIT,GL_NEAREST);
 glReadBuffer(sourceRead);glBindFramebuffer(GL_READ_FRAMEBUFFER,read);glReadBuffer(readBuffer);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glBindTexture(GL_TEXTURE_2D,texture);
 if(scissor)glEnable(GL_SCISSOR_TEST);if(srgb)glEnable(GL_FRAMEBUFFER_SRGB);return ok;
}

#include "inline_hud.h"
using SwapFn=BOOL(WINAPI*)(HDC);SwapFn originalSwap=nullptr;
EyeTarget screenTarget,menuScene;
// Preserve the native map scene for its later Lua markers while rendering the
// surrounding world from the tracked player camera into the two headset eyes.
struct TabletBackdrop {
 bool saved=false;GLint destination=0,viewport[4]{};
 bool Save(){glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&destination);glGetIntegerv(GL_VIEWPORT,viewport);saved=Capture(menuScene);return saved;}
 void Restore(){
  if(!saved)return;saved=false;GLint draw=0,read=0;glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);
  bool scissor=glIsEnabled(GL_SCISSOR_TEST),srgb=glIsEnabled(GL_FRAMEBUFFER_SRGB);glDisable(GL_SCISSOR_TEST);glDisable(GL_FRAMEBUFFER_SRGB);
  glBindFramebuffer(GL_READ_FRAMEBUFFER,menuScene.fbo);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,destination);
  glBlitFramebuffer(0,0,menuScene.width,menuScene.height,viewport[0],viewport[1],viewport[0]+viewport[2],viewport[1]+viewport[3],GL_COLOR_BUFFER_BIT,GL_NEAREST);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glBindFramebuffer(GL_READ_FRAMEBUFFER,read);if(scissor)glEnable(GL_SCISSOR_TEST);if(srgb)glEnable(GL_FRAMEBUFFER_SRGB);
 }
 ~TabletBackdrop(){Restore();}
};
void CaptureInteractiveMenu(HWND window){
 RECT rect{};if(!GetClientRect(window,&rect)||rect.right<1||rect.bottom<1)return;
 GLint draw=0,read=0,viewport[4]{},scissor[4]{};GLfloat clear[4]{};GLboolean mask[4]{};
 glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_SCISSOR_BOX,scissor);glGetFloatv(GL_COLOR_CLEAR_VALUE,clear);glGetBooleanv(GL_COLOR_WRITEMASK,mask);
 bool clipping=glIsEnabled(GL_SCISSOR_TEST)!=0;glDisable(GL_SCISSOR_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);glViewport(0,0,rect.right,rect.bottom);
 bool captured=Capture(screenTarget);
 if(captured){
  GamePointer::Present(screenTarget.texture);
  for(int eye=0;eye<2&&!TabletUI::Active();eye++){
   auto& target=targets[eye];if(!target.fbo){glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);Capture(target);}
   // The native menu texture is on an absolute SteamVR panel. Do not also
   // paste it into the head-relative eye images beneath that panel.
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER,target.fbo);glClearColor(.02f,.02f,.02f,1);glClear(GL_COLOR_BUFFER_BIT);
  }
 }
 glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glBindFramebuffer(GL_READ_FRAMEBUFFER,read);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glScissor(scissor[0],scissor[1],scissor[2],scissor[3]);glClearColor(clear[0],clear[1],clear[2],clear[3]);glColorMask(mask[0],mask[1],mask[2],mask[3]);if(clipping)glEnable(GL_SCISSOR_TEST);
 if(captured)InlineHUD::Begin(false);
}
BOOL WINAPI SwapHook(HDC dc){
 Phase("swap: controller/menu update");
 if(!enabled&&systemVR){GamePointer::Release();GamePointer::HidePanel();HUDMath::interactive=false;}
 if(enabled&&glReady&&systemVR){
  VRMenu::gameWindow=WindowFromDC(dc);std::array<vr::TrackedDevicePose_t,vr::k_unMaxTrackedDeviceCount> poses{};
  if(InlineHUD::pending)compositor->GetLastPoses(poses.data(),unsigned(poses.size()),nullptr,0);else compositor->WaitGetPoses(poses.data(),unsigned(poses.size()),nullptr,0);
  if(poses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid){auto head=Stereo::Pose(poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);ControllerBridge::Sample(poses,centerInverse,centerSerial);VRMenu::Update(poses,head);GamePointer::Update(poses,head,VRMenu::gameWindow);}else{GamePointer::Release();GamePointer::HidePanel();}
 }
 if(enabled&&glReady&&HUDMath::interactive){Phase("game menu capture");CaptureInteractiveMenu(VRMenu::gameWindow);}
 Phase("swap: VR submit");InlineHUD::Submit();
 if(enabled&&glReady&&systemVR&&VRMenu::screenMode&&ControllerBridge::menuOpen){
  VRMenu::gameWindow=WindowFromDC(dc);RECT rect{};
  if(GetClientRect(VRMenu::gameWindow,&rect)&&rect.right>0&&rect.bottom>0){
   GLint draw=0,viewport[4]{};glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_VIEWPORT,viewport);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);glViewport(0,0,rect.right,rect.bottom);
   if(Capture(screenTarget)){glFlush();vr::Texture_t texture{reinterpret_cast<void*>(static_cast<std::uintptr_t>(screenTarget.texture)),vr::TextureType_OpenGL,vr::ColorSpace_Auto};vr::VROverlay()->SetOverlayTexture(VRMenu::screen,&texture);}
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
  }
 }else if(!enabled&&systemVR&&ControllerBridge::menuOpen)VRMenu::Hide();
 Phase("desktop swap");auto swapStart=GetTickCount64();auto result=originalSwap(dc);
 if(GetTickCount64()-swapStart>250)Log("Slow desktop swap ms="+std::to_string(GetTickCount64()-swapStart));Phase("game update between rendered frames");return result;
}
double NowMs(){LARGE_INTEGER n{};QueryPerformanceCounter(&n);static double scale=[](){LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return 1000.0/double(f.QuadPart);}();return double(n.QuadPart)*scale;}
struct FrameTiming {double last=0,report=0,gapMax=0,waitMax=0,renderMax=0,captureMax=0;unsigned count=0;} timing;
void Hook(void* renderer,void* target,void* scene,Stereo::Camera* camera) {
 static thread_local bool recursive=false;
 if(recursive){original(renderer,target,scene,camera);return;}
 // Some presentation paths bypass gdi32 SwapBuffers. Submit the previous
 // completed frame here if necessary, rather than leaving the headset blank.
 Phase("render: pending VR submit");InlineHUD::Submit();
 recursive=true;struct Reset {bool& flag;~Reset(){flag=false;}} reset{recursive};
 ++frames;
 const bool readable=Readable(camera,sizeof(*camera));
 const bool valid=readable&&Stereo::Valid(*camera);
 static bool reportedReady=false;
 const bool firstReady=!reportedReady&&valid&&wglGetCurrentContext()!=nullptr;
 if(firstReady)reportedReady=true;
 if(frames==1||frames==120||firstReady){
  Log("Render callback "+std::to_string(frames)+" cameraValid="+std::to_string(valid)+" OpenGLContext="+std::to_string(wglGetCurrentContext()!=nullptr));
  if(valid)Log("Camera near="+std::to_string(camera->nearClip)+" far="+std::to_string(camera->farClip));
 }
 if(!enabled.load()||!valid||!wglGetCurrentContext()){original(renderer,target,scene,camera);return;}
 if(!glReady){glewExperimental=GL_TRUE;auto e=glewInit();glReady=e==GLEW_OK&&GLEW_VERSION_3_0;if(!glReady){Log("Required OpenGL framebuffer API unavailable");enabled=false;original(renderer,target,scene,camera);return;}}
 if(!InitVR()){original(renderer,target,scene,camera);return;}
 // Map geometry and its later Lua markers share the native camera. Applying
 // a headset camera here separates map markers from their native positions.
 bool tabletMenu=HUDMath::interactive&&TabletUI::Active()&&ControllerBridge::Accessories::page>0;
 if((HUDMath::interactive&&!tabletMenu)||(ControllerBridge::menuOpen&&GamePointer::manual)){Phase("native menu/map render");original(renderer,target,scene,camera);return;}
 TabletBackdrop backdrop;
 if(tabletMenu){Phase("tablet native menu scene");original(renderer,target,scene,camera);if(!backdrop.Save())return;}
 double frameStart=NowMs();if(timing.last)timing.gapMax=(std::max)(timing.gapMax,frameStart-timing.last);timing.last=frameStart;
 Phase("SteamVR events/recenter");CheckVRRecenter();
 std::array<vr::TrackedDevicePose_t,vr::k_unMaxTrackedDeviceCount> poses{};
 Phase("SteamVR WaitGetPoses");auto poseError=compositor->WaitGetPoses(poses.data(),static_cast<std::uint32_t>(poses.size()),nullptr,0);
 double waited=NowMs()-frameStart;timing.waitMax=(std::max)(timing.waitMax,waited);
 if(poseError!=vr::VRCompositorError_None||!poses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid){original(renderer,target,scene,camera);return;}
 auto head=Stereo::Pose(poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
 static bool recenterHeld=false;
 DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
 const bool recenterDown=foregroundPid==GetCurrentProcessId()&&(GetAsyncKeyState(VK_F8)&0x8000)!=0;
 const bool recenterPressed=recenterDown&&!recenterHeld;recenterHeld=recenterDown;
 if(!centerSet||recenterPressed||ControllerBridge::recenterRequested.exchange(false)){centerInverse=glm::inverse(Stereo::Upright(head));centerSet=true;++centerSerial;Log("Head position and heading recentered; gravity preserved (F8 to recenter)");}
 Phase("controller state sampling");ControllerBridge::Sample(poses,centerInverse,centerSerial);
 Stereo::Camera gameCamera=*camera;
 if(tabletMenu){auto player=static_cast<std::uint8_t*>(ControllerBridge::LocalPlayer());Controllers::Transform pose{};if(Readable(player,0x65C)){std::memcpy(&pose,player+0x640,sizeof(pose));if(Controllers::Valid(pose))gameCamera.view=glm::inverse(Controllers::Matrix(pose));}}
 gameCamera.view=glm::inverse(ControllerBridge::BasePose(glm::inverse(gameCamera.view)));
 Phase("character/tool render preparation");ControllerBridge::HandsOnlyVisual::Update(ControllerBridge::LocalPlayer());
 bool captured[2]{};
 const bool onePass=VRMenu::singlePass;
 for(int eye=0;eye<(onePass?1:2);++eye){
  auto which=static_cast<vr::EVREye>(eye);
  auto projection=systemVR->GetProjectionMatrix(which,gameCamera.nearClip,gameCamera.farClip);
  if(ControllerBridge::widerView){projection.m[0][0]/=1.35f;projection.m[1][1]/=1.35f;}
  auto adjusted=Stereo::EyeCamera(gameCamera,centerInverse*head,Stereo::Pose(systemVR->GetEyeToHeadTransform(which)),projection);
  if(onePass)adjusted=Stereo::EyeCamera(gameCamera,centerInverse*head,glm::mat4(1.f),projection);
  Phase("native eye render");double before=NowMs();original(renderer,target,scene,&adjusted);timing.renderMax=(std::max)(timing.renderMax,NowMs()-before);
  before=NowMs();captured[eye]=Capture(targets[eye]);
  InlineHUD::eyeViewport[eye]=HUDMath::Viewport(targets[eye].width,targets[eye].height,projection.m[0][0],projection.m[1][1],projection.m[0][2],projection.m[1][2],onePass?0.f:systemVR->GetEyeToHeadTransform(which).m[0][3]);timing.captureMax=(std::max)(timing.captureMax,NowMs()-before);
 }
 if(onePass)captured[1]=captured[0];
 backdrop.Restore();
 if(captured[0]&&captured[1])InlineHUD::Begin(onePass);
 else{Log("Eye capture incomplete; stereo disabled");enabled=false;}

 if(++timing.count&&frameStart-timing.report>5000){Log("Timing max ms: frame-gap="+std::to_string(timing.gapMax)+" wait="+std::to_string(timing.waitMax)+" render-eye="+std::to_string(timing.renderMax)+" capture="+std::to_string(timing.captureMax));timing.report=frameStart;timing.gapMax=timing.waitMax=timing.renderMax=timing.captureMax=0;timing.count=0;}
 Phase("game UI after scene render");
}
DWORD WINAPI Start(void*) {
 wchar_t path[MAX_PATH]{};GetModuleFileNameW(self,path,MAX_PATH);logFile.open(std::filesystem::path(path).parent_path()/"SledgePort21.log",std::ios::trunc);
 HANDLE logger=CreateThread(nullptr,0,LogWorker,nullptr,0,nullptr);if(logger)CloseHandle(logger);
 Log("Sledge 2.1.0 experimental renderer bridge r7.20: synchronized avatar hand targets, connected arms, compact thigh tablet and native swing attacks");
 auto base=reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
 if(!CurrentBuild::Matches(base,nt->OptionalHeader.SizeOfImage)){Log("REFUSED: executable differs from captured 2.1.0 build. No hooks installed.");return 1;}
 // The one-click launcher injects before GL initialization. Wait on this
 // startup worker, not the game thread, before capturing the draw pointers.
 Log("Waiting for OpenGL draw entry points before installing HUD hooks");
 for(unsigned n=0;n<1800;n++){
  if(*reinterpret_cast<void**>(base+0x1F88230)&&*reinterpret_cast<void**>(base+0x1F8D270)&&*reinterpret_cast<void**>(base+0x1F8D268))break;
  Sleep(100);
 }
 if(!*reinterpret_cast<void**>(base+0x1F88230)){Log("HUD hooks not ready: OpenGL did not initialize; restart with -gfxapi OpenGL");return 4;}
 HMODULE pinned{};GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&Start),&pinned);
 original=reinterpret_cast<RenderFn>(base+CurrentBuild::RenderSceneRva);
 ControllerBridge::Prepare(base);VRMenu::LoadSettings();
 originalSwap=reinterpret_cast<SwapFn>(GetProcAddress(GetModuleHandleW(L"gdi32.dll"),"SwapBuffers"));
 auto result=DetourTransactionBegin();if(result!=NO_ERROR){Log("Detour transaction failed");return 2;}
 DetourUpdateThread(GetCurrentThread());std::vector<HANDLE> threads;
 HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);THREADENTRY32 t{sizeof(t)};bool failed=snap==INVALID_HANDLE_VALUE;
 if(!failed&&Thread32First(snap,&t))do{if(t.th32OwnerProcessID==GetCurrentProcessId()&&t.th32ThreadID!=GetCurrentThreadId()){
  HANDLE h=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,t.th32ThreadID);
  if(!h){failed=true;break;}threads.push_back(h);if(DetourUpdateThread(h)!=NO_ERROR){failed=true;break;}
 }}while(Thread32Next(snap,&t));
 if(snap!=INVALID_HANDLE_VALUE)CloseHandle(snap);
 if(failed){DetourTransactionAbort();result=ERROR_ACCESS_DENIED;}else{result=DetourAttach(reinterpret_cast<PVOID*>(&original),reinterpret_cast<PVOID>(&Hook));if(result==NO_ERROR)result=ControllerBridge::Attach();if(result==NO_ERROR)result=InlineHUD::Attach(base);if(result==NO_ERROR)result=originalSwap?DetourAttach(reinterpret_cast<PVOID*>(&originalSwap),reinterpret_cast<PVOID>(&SwapHook)):ERROR_PROC_NOT_FOUND;if(result==NO_ERROR)result=DetourTransactionCommit();else DetourTransactionAbort();}
 for(auto h:threads)CloseHandle(h);
 if(result!=NO_ERROR){Log("Hook installation failed: "+std::to_string(result));return 3;}
 HANDLE inputThread=CreateThread(nullptr,0,ControllerBridge::InputThread,nullptr,0,nullptr);if(inputThread)CloseHandle(inputThread);else Log("Controller input worker could not start");
 ready=true;Log("Scene hook installed at +0x87F40; camera passes through unchanged until stereo is explicitly enabled.");return 0;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI SledgePortEnableStereo(void*) {if(!ready)return ERROR_NOT_READY;enabled=true;return 0;}
extern "C" __declspec(dllexport) DWORD WINAPI SledgePortDisableStereo(void*) {enabled=false;return 0;}
BOOL WINAPI DllMain(HMODULE module,DWORD reason,LPVOID) {if(reason==DLL_PROCESS_ATTACH){self=module;DisableThreadLibraryCalls(module);HANDLE h=CreateThread(nullptr,0,Start,nullptr,0,nullptr);if(h)CloseHandle(h);}return TRUE;}
