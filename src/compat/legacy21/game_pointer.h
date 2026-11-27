#pragma once
#include "menu_math.h"
namespace GamePointer {
auto& active=HUDMath::interactive;bool manual=false,mouseDown=false,triggerHeld=false,escapeHeld=false,sawNative=false,xHeld=false;auto& u=HUDMath::pointerU;auto& v=HUDMath::pointerV;auto& visible=HUDMath::pointerVisible;
Controllers::WheelRepeat scrollRepeat;ULONGLONG lastNativeCursor=0;
bool wasTablet=false;
ULONGLONG ignoreEscapeUntil=0;glm::mat4 placement(1.f);bool anchored=false;
vr::VROverlayHandle_t panel=0,pointer=0;float PanelWidth=1.9f;float panelHeight=PanelWidth*9.f/16.f;
vr::HmdMatrix34_t Matrix34(const glm::mat4& m){vr::HmdMatrix34_t out{};for(int r=0;r<3;r++)for(int c=0;c<4;c++)out.m[r][c]=m[c][r];return out;}
bool Init(){
 if(panel&&pointer)return true;
 auto overlay=vr::VROverlay();if(!overlay)return false;
 std::string key="sledgeport.native-menu."+std::to_string(GetCurrentProcessId());
 if(!panel&&overlay->CreateOverlay(key.c_str(),"Teardown map and menus",&panel)!=vr::VROverlayError_None)return false;
 key+=".cursor";if(!pointer&&overlay->CreateOverlay(key.c_str(),"Menu pointer",&pointer)!=vr::VROverlayError_None)return false;
 static std::array<unsigned char,32*32*4> pixels{};static bool initialized=false;
 if(!initialized){for(int y=0;y<32;y++)for(int x=0;x<32;x++){float r=std::hypot(float(x)-15.5f,float(y)-15.5f);int i=(y*32+x)*4;if(r<14){pixels[i]=pixels[i+1]=pixels[i+2]=r<10?255:0;pixels[i+3]=255;}}overlay->SetOverlayRaw(pointer,pixels.data(),32,32,4);initialized=true;}
 overlay->SetOverlayWidthInMeters(panel,PanelWidth);overlay->SetOverlayWidthInMeters(pointer,.022f);overlay->SetOverlaySortOrder(panel,10);overlay->SetOverlaySortOrder(pointer,100);return true;
}
void HidePanel(){TabletUI::pointing=false;TabletUI::lastHome=false;TabletUI::Keyboard(glm::mat4(1),false);if(vr::VROverlay()){if(panel)vr::VROverlay()->HideOverlay(panel);if(pointer)vr::VROverlay()->HideOverlay(pointer);}anchored=false;}
// OpenVR handles OpenGL texture orientation. Reversing V here flips the
// native menu/map again and reflects its lettering relative to the pointer.
void Present(GLuint texture){if(!active||!anchored||!panel||(TabletUI::Active()&&ControllerBridge::Accessories::page==0))return;vr::Texture_t image{reinterpret_cast<void*>(static_cast<std::uintptr_t>(texture)),vr::TextureType_OpenGL,vr::ColorSpace_Auto};vr::VRTextureBounds_t bounds{};bounds.uMin=0;bounds.vMin=0;bounds.uMax=1;bounds.vMax=1;vr::VROverlay()->SetOverlayTextureBounds(panel,&bounds);vr::VROverlay()->SetOverlayTexture(panel,&image);vr::VROverlay()->ShowOverlay(panel);}
void Release(){if(mouseDown){INPUT i{};i.type=INPUT_MOUSE;i.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&i,sizeof(i));mouseDown=false;}triggerHeld=false;visible=false;if(pointer&&vr::VROverlay())vr::VROverlay()->HideOverlay(pointer);}
void Update(const std::array<vr::TrackedDevicePose_t,vr::k_unMaxTrackedDeviceCount>& poses,const glm::mat4& head,HWND window){
 bool tablet=TabletUI::Active();bool home=tablet&&ControllerBridge::Accessories::page==0;bool typing=tablet&&ControllerBridge::Accessories::page==2&&ControllerBridge::Accessories::keyboard;
 CURSORINFO cursor{sizeof(cursor)};bool native=GetCursorInfo(&cursor)&&(cursor.flags&CURSOR_SHOWING);
 DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
 bool escape=pid==GetCurrentProcessId()&&(GetAsyncKeyState(VK_ESCAPE)&0x8000);if(escape&&!escapeHeld&&GetTickCount64()>=ignoreEscapeUntil)manual=!active.load();escapeHeld=escape;
 if(native){sawNative=true;lastNativeCursor=GetTickCount64();}else if(!tablet&&sawNative&&GetTickCount64()-lastNativeCursor>500){manual=false;sawNative=false;TabletUI::suspended=false;}
 bool usable=enabled&&window&&pid==GetCurrentProcessId()&&systemVR->IsInputAvailable()&&!ControllerBridge::menuOpen;
 auto player=static_cast<std::uint8_t*>(ControllerBridge::LocalPlayer());bool computer=Readable(player,0x9A8)&&*reinterpret_cast<void**>(player+0x9A0)!=nullptr;
 bool wasActive=active.load()||wasTablet;if(wasTablet&&!tablet)anchored=false;wasTablet=usable&&tablet;active=usable&&(tablet?!home:(native||sawNative||manual||computer));HUDMath::tabletInteractive=active&&tablet;if(!active&&!(usable&&tablet)){Release();HidePanel();xHeld=false;scrollRepeat.Update(0,true,GetTickCount64());return;}
 RECT rect{};if(!GetClientRect(window,&rect)||rect.right<=0||rect.bottom<=0||!Init()){active=false;Release();HidePanel();return;}
 PanelWidth=tablet?(typing?.18f:.24f):1.9f;panelHeight=home?.15f:PanelWidth*float(rect.bottom)/rect.right;
 vr::VROverlay()->SetOverlayWidthInMeters(panel,PanelWidth);
 glm::mat4 tabletPose(1);
 if(tablet){if(!TabletUI::Pose(poses,tabletPose)){Release();HidePanel();return;}placement=tabletPose;if(typing)placement=placement*glm::translate(glm::mat4(1),glm::vec3(0,.03f,0));auto m=Matrix34(placement);vr::VROverlay()->SetOverlayTransformAbsolute(panel,vr::TrackingUniverseStanding,&m);anchored=true;TabletUI::Keyboard(tabletPose,typing);if(home){if(!TabletUI::lastHome)TabletUI::Draw(panel,false);vr::VROverlay()->ShowOverlay(panel);}TabletUI::lastHome=home;}
 else{TabletUI::Keyboard(glm::mat4(1),false);TabletUI::lastHome=false;}

 if(!anchored){placement=Stereo::Upright(head)*glm::translate(glm::mat4(1),glm::vec3(0,0,-2));auto transform=Matrix34(placement);vr::VROverlay()->SetOverlayTransformAbsolute(panel,vr::TrackingUniverseStanding,&transform);anchored=true;}
 auto leftId=systemVR->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);vr::VRControllerState_t left{};
 bool leftValid=leftId<poses.size()&&systemVR->GetControllerState(leftId,&left,sizeof(left));bool x=leftValid&&(left.ulButtonPressed&vr::ButtonMaskFromId(vr::k_EButton_A));
 if(!wasActive)xHeld=x; // The press that opened the computer must not close it.
 if(x&&!xHeld&&!home){INPUT keys[2]{};keys[0].type=keys[1].type=INPUT_KEYBOARD;keys[0].ki.wVk=keys[1].ki.wVk=tablet&&ControllerBridge::Accessories::page==1?VK_TAB:(computer?'E':VK_ESCAPE);keys[1].ki.dwFlags=KEYEVENTF_KEYUP;SendInput(2,keys,sizeof(INPUT));manual=false;if(tablet)ControllerBridge::Accessories::page=0;}xHeld=x;
 int pointerHand=tablet?1-ControllerBridge::Accessories::tabletHand.load():1;int id=systemVR->GetTrackedDeviceIndexForControllerRole(pointerHand?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand);vr::VRControllerState_t state{};
 if(id<0||id>=int(poses.size())||!poses[id].bPoseIsValid||!systemVR->GetControllerState(id,&state,sizeof(state))){Release();return;}
 auto hand=Stereo::Pose(poses[id].mDeviceToAbsoluteTracking);
 bool pressed=state.rAxis[1].x>.55f;
 if(!wasActive)triggerHeld=pressed;
 MenuMath::Hit hit;bool keyHit=typing&&MenuMath::Intersect(TabletUI::keyboardPose,hand,.24f,.04875f,hit);
 bool inside=keyHit||MenuMath::Intersect(placement,hand,PanelWidth,panelHeight,hit);TabletUI::pointing=tablet&&inside;
 if(!inside){Release();vr::VROverlay()->HideOverlay(pointer);triggerHeld=pressed;return;}
 u=hit.u;v=hit.v;visible=false; // Cursor is on the anchored panel, not the eye HUD.
 auto cursorPose=keyHit?TabletUI::keyboardPose:placement;cursorPose[3]=glm::vec4(hit.world,1)+cursorPose[2]*.001f;vr::VROverlay()->SetOverlayWidthInMeters(pointer,tablet?.006f:.022f);auto cursorMatrix=Matrix34(cursorPose);vr::VROverlay()->SetOverlayTransformAbsolute(pointer,vr::TrackingUniverseStanding,&cursorMatrix);vr::VROverlay()->ShowOverlay(pointer);
 if(home||keyHit){bool clicked=pressed&&!triggerHeld;if(mouseDown)Release();triggerHeld=pressed;
  if(clicked){if(keyHit)TabletUI::Key(hit.u,hit.v);else if(hit.u>=.03125f&&hit.u<=.96875f){if(hit.v>=.125f&&hit.v<=.425f){ControllerBridge::Accessories::page=1;ControllerBridge::mapRequested=true;}else if(hit.v>=.55f&&hit.v<=.85f){ControllerBridge::Accessories::page=2;ControllerBridge::Accessories::spawnRequest=1;}TabletUI::lastHome=false;}ControllerBridge::Haptics::Grab(pointerHand);}return;}
 POINT point{LONG(u*rect.right),LONG(v*rect.bottom)};ClientToScreen(window,&point);
 INPUT input{};input.type=INPUT_MOUSE;input.mi.dwFlags=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK;
 input.mi.dx=LONG((point.x-GetSystemMetrics(SM_XVIRTUALSCREEN))*65535.0/(std::max)(1,GetSystemMetrics(SM_CXVIRTUALSCREEN)-1));input.mi.dy=LONG((point.y-GetSystemMetrics(SM_YVIRTUALSCREEN))*65535.0/(std::max)(1,GetSystemMetrics(SM_CYVIRTUALSCREEN)-1));SendInput(1,&input,sizeof(input));
 int scroll=Controllers::MenuScroll(leftValid?left.rAxis[0].y:0.f,state.rAxis[0].y);
 if(scrollRepeat.Update(scroll,true,GetTickCount64())){INPUT wheel{};wheel.type=INPUT_MOUSE;wheel.mi.dwFlags=MOUSEEVENTF_WHEEL;wheel.mi.mouseData=DWORD(scroll*WHEEL_DELTA);SendInput(1,&wheel,sizeof(wheel));}
 if(pressed&&!triggerHeld){input={};input.type=INPUT_MOUSE;input.mi.dwFlags=MOUSEEVENTF_LEFTDOWN;mouseDown=SendInput(1,&input,sizeof(input))==1;}
 if(!pressed&&mouseDown){input={};input.type=INPUT_MOUSE;input.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&input,sizeof(input));mouseDown=false;}
 triggerHeld=pressed;
}
}
