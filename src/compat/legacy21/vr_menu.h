#pragma once
namespace VRMenu {
vr::VROverlayHandle_t panel=vr::k_ulOverlayHandleInvalid,screen=vr::k_ulOverlayHandleInvalid,cursor=vr::k_ulOverlayHandleInvalid;
std::atomic<bool> singlePass{false};
bool screenMode=false,mouseHeld=false,dirty=true;
Controllers::BButtonGesture bGesture;
bool triggerHeld[2]{};int pointerHand=1;ULONGLONG lastHoverTime=0;
int hover=-1;glm::mat4 placement(1.f);HWND gameWindow=nullptr;
constexpr int Width=MenuMath::Width,Height=MenuMath::Height;
std::array<unsigned char,Width*Height*4> panelPixels{};
vr::HmdMatrix34_t Matrix34(const glm::mat4& m){vr::HmdMatrix34_t out{};for(int r=0;r<3;r++)for(int c=0;c<4;c++)out.m[r][c]=m[c][r];return out;}
std::wstring SettingsPath(){wchar_t path[MAX_PATH]{};GetModuleFileNameW(self,path,MAX_PATH);return (std::filesystem::path(path).parent_path()/"vr-settings.ini").wstring();}
void LoadSettings(){auto path=SettingsPath();ControllerBridge::physicalMelee=GetPrivateProfileIntW(L"VR",L"PhysicalMelee",1,path.c_str())!=0;ControllerBridge::widerView=GetPrivateProfileIntW(L"VR",L"WiderView",0,path.c_str())!=0;ControllerBridge::vehicleTurnSpeed=float((std::clamp)(int(GetPrivateProfileIntW(L"VR",L"VehicleTurnSpeed",90,path.c_str())),15,720));ControllerBridge::handsOnly=GetPrivateProfileIntW(L"VR",L"HandsOnly",0,path.c_str())!=0;ControllerBridge::physicalContact=GetPrivateProfileIntW(L"VR",L"PhysicalContact",1,path.c_str())!=0;HUDMath::visible=GetPrivateProfileIntW(L"VR",L"ShowHUD",1,path.c_str())!=0;ControllerBridge::separateGrab=GetPrivateProfileIntW(L"VR",L"SeparateGrab",1,path.c_str())!=0;ControllerBridge::headForward=float((std::clamp)(int(GetPrivateProfileIntW(L"VR",L"HeadForwardCentimeters",10,path.c_str())),0,30))*.01f;ControllerBridge::turnSpeed=float((std::clamp)(int(GetPrivateProfileIntW(L"VR",L"SmoothTurnSpeed",150,path.c_str())),15,720));}
void SaveSettings(){auto path=SettingsPath();WritePrivateProfileStringW(L"VR",L"PhysicalMelee",ControllerBridge::physicalMelee?L"1":L"0",path.c_str());WritePrivateProfileStringW(L"VR",L"WiderView",ControllerBridge::widerView?L"1":L"0",path.c_str());auto vehicle=std::to_wstring(int(ControllerBridge::vehicleTurnSpeed.load()));WritePrivateProfileStringW(L"VR",L"VehicleTurnSpeed",vehicle.c_str(),path.c_str());WritePrivateProfileStringW(L"VR",L"HandsOnly",ControllerBridge::handsOnly?L"1":L"0",path.c_str());WritePrivateProfileStringW(L"VR",L"PhysicalContact",ControllerBridge::physicalContact?L"1":L"0",path.c_str());WritePrivateProfileStringW(L"VR",L"ShowHUD",HUDMath::visible?L"1":L"0",path.c_str());WritePrivateProfileStringW(L"VR",L"SeparateGrab",ControllerBridge::separateGrab?L"1":L"0",path.c_str());auto value=std::to_wstring(int(ControllerBridge::turnSpeed.load()));WritePrivateProfileStringW(L"VR",L"SmoothTurnSpeed",value.c_str(),path.c_str());}
void MouseRelease(){if(mouseHeld){INPUT in{};in.type=INPUT_MOUSE;in.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&in,sizeof(in));mouseHeld=false;}}
void Hide(){triggerHeld[0]=triggerHeld[1]=false;hover=-1;if(vr::VROverlay()){if(panel)vr::VROverlay()->HideOverlay(panel);if(screen)vr::VROverlay()->HideOverlay(screen);if(cursor)vr::VROverlay()->HideOverlay(cursor);}MouseRelease();ControllerBridge::menuOpen=false;screenMode=false;}
bool Init(){
 if(panel&&screen&&cursor)return true;auto overlay=vr::VROverlay();if(!overlay)return false;
 std::string key="sledgeport.settings."+std::to_string(GetCurrentProcessId());
 if(!panel&&overlay->CreateOverlay(key.c_str(),"Sledge VR settings",&panel)!=vr::VROverlayError_None)return false;
 key+=".game";if(!screen&&overlay->CreateOverlay(key.c_str(),"Teardown menus and HUD",&screen)!=vr::VROverlayError_None)return false;
 key+=".pointer";if(!cursor&&overlay->CreateOverlay(key.c_str(),"VR pointer",&cursor)!=vr::VROverlayError_None)return false;
 std::array<unsigned char,32*32*4> dot{};
 for(int y=0;y<32;y++)for(int x=0;x<32;x++){float r=std::hypot(float(x)-15.5f,float(y)-15.5f);auto i=(y*32+x)*4;if(r<14.f){bool edge=r>10.f;dot[i]=edge?0:255;dot[i+1]=edge?0:255;dot[i+2]=edge?0:255;dot[i+3]=255;}}
 overlay->SetOverlayRaw(cursor,dot.data(),32,32,4);overlay->SetOverlayWidthInMeters(cursor,.026f);overlay->SetOverlaySortOrder(cursor,100);
 overlay->SetOverlayWidthInMeters(panel,1.6f);overlay->SetOverlayWidthInMeters(screen,1.8f);
 return true;
}
void Show(const glm::mat4& head){triggerHeld[0]=triggerHeld[1]=false;hover=-1;pointerHand=1;
 if(!Init()){Log("VR settings overlay creation failed");return;}
 placement=Stereo::Upright(head)*glm::translate(glm::mat4(1.f),glm::vec3(0,0,-1.6f));
 auto pose=Matrix34(placement);vr::VROverlay()->SetOverlayTransformAbsolute(panel,vr::TrackingUniverseStanding,&pose);vr::VROverlay()->SetOverlayTransformAbsolute(screen,vr::TrackingUniverseStanding,&pose);
 screenMode=false;ControllerBridge::menuOpen=true;dirty=true;vr::VROverlay()->ShowOverlay(panel);
}
void Draw(){
 if(!dirty||!panel)return;dirty=false;
 HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=Width;info.bmiHeader.biHeight=-Height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
 if(!dc||!bitmap||!pixels){if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);return;}
 auto oldBitmap=SelectObject(dc,bitmap);RECT rect{0,0,Width,Height};HBRUSH background=CreateSolidBrush(RGB(18,23,32));FillRect(dc,&rect,background);DeleteObject(background);
 HFONT font=CreateFontW(25,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");auto oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(235,242,250));
 auto label=[&](int x,int y,const std::wstring& text){TextOutW(dc,x,y,text.c_str(),int(text.size()));};
 label(28,20,L"Sledge VR settings");label(28,54,L"Point either controller and squeeze its trigger.");
 std::wstring rows[]={ControllerBridge::headFollow?L"Head follows view: ON":L"Head follows view: OFF",L"Smooth turn speed: "+std::to_wstring(int(ControllerBridge::turnSpeed.load()))+L" degrees/sec",L"Recenter position and heading",L"Open game pause menu",GamePointer::manual?L"Game cursor: ON (click to exit)":L"Point at game menus / campaign computer",singlePass?L"Rendering: single-pass compatibility (no stereo depth)":L"Rendering: stereoscopic",HUDMath::visible?L"VR HUD: ON (click to disable)":L"VR HUD: OFF (click to enable)",ControllerBridge::separateGrab?L"Grab mode: separate hands":L"Grab mode: normal game grab",ControllerBridge::physicalContact?L"Experimental physical contact: ON":L"Experimental physical contact: OFF",ControllerBridge::handsOnly?L"Hands only: ON":L"Hands only: OFF",L"Vehicle turn speed: "+std::to_wstring(int(ControllerBridge::vehicleTurnSpeed.load())),L"Adjust vehicle camera (3 seconds)",L"Reset vehicle camera height",ControllerBridge::PlayerAssists::godMode?L"God mode: ON":L"God mode: OFF",ControllerBridge::PlayerAssists::unlimitedAmmo?L"Unlimited native ammo: ON":L"Unlimited native ammo: OFF",L"Retrieve holstered tool",L"Return tablet to home",ControllerBridge::physicalMelee?L"Physical melee: ON":L"Physical melee: OFF",ControllerBridge::widerView?L"Wider VR view":L"Normal VR view",ControllerBridge::Accessories::meleeMode==1?L"Swing attack for this tool: ON":(ControllerBridge::Accessories::meleeMode==2?L"Swing attack for this tool: OFF":L"Swing attack for this tool: AUTO"),L"Close settings"};
 for(int i=0;i<MenuMath::Count;i++){auto cell=MenuMath::Cell(i);RECT row{cell.left,cell.top,cell.right,cell.bottom};HBRUSH brush=CreateSolidBrush(RGB(37,46,62));FillRect(dc,&row,brush);DeleteObject(brush);RECT text{row.left+18,row.top+12,row.right-18,row.bottom-10};DrawTextW(dc,rows[i].c_str(),int(rows[i].size()),&text,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);}
 label(28,Height-38,L"B: settings   Double B: pause   Hold B: map");
 GdiFlush();auto bytes=static_cast<unsigned char*>(pixels);for(int i=0;i<Width*Height;i++){std::swap(bytes[i*4],bytes[i*4+2]);bytes[i*4+3]=255;}
 std::memcpy(panelPixels.data(),pixels,panelPixels.size());vr::VROverlay()->SetOverlayRaw(panel,panelPixels.data(),Width,Height,4);
 SelectObject(dc,oldFont);SelectObject(dc,oldBitmap);DeleteObject(font);DeleteObject(bitmap);DeleteDC(dc);
}
void Escape(){INPUT keys[2]{};keys[0].type=keys[1].type=INPUT_KEYBOARD;keys[0].ki.wVk=keys[1].ki.wVk=VK_ESCAPE;keys[1].ki.dwFlags=KEYEVENTF_KEYUP;SendInput(2,keys,sizeof(INPUT));}
void Click(int row){
 if(row==0){ControllerBridge::headFollow=!ControllerBridge::headFollow;ControllerBridge::headingReady=false;}
 if(row==1){ControllerBridge::turnSpeed=Controllers::NextTurnSpeed(ControllerBridge::turnSpeed);SaveSettings();}
 if(row==2){ControllerBridge::recenterRequested=true;Hide();}
 if(row==3){Hide();TabletUI::suspended=true;GamePointer::manual=true;Escape();}
 if(row==4){GamePointer::manual=!GamePointer::manual;Hide();}
 if(row==5)singlePass=!singlePass;
 if(row==6){HUDMath::visible=!HUDMath::visible;SaveSettings();}
 if(row==7){ControllerBridge::separateGrab=!ControllerBridge::separateGrab;SaveSettings();}
 if(row==8){ControllerBridge::physicalContact=!ControllerBridge::physicalContact;SaveSettings();}
 if(row==9){bool wanted=!ControllerBridge::handsOnly;ControllerBridge::handsOnly=wanted;if(wanted)ControllerBridge::separateGrab=true;ControllerBridge::VRHandsTool::request=wanted?1:-1;SaveSettings();}
 if(row==10){ControllerBridge::vehicleTurnSpeed=Controllers::NextTurnSpeed(ControllerBridge::vehicleTurnSpeed);SaveSettings();}
 if(row==11){ControllerBridge::VehicleCamera::Request();Hide();}
 if(row==12)ControllerBridge::vehicleHeight=0;
 if(row==13)ControllerBridge::PlayerAssists::godMode=!ControllerBridge::PlayerAssists::godMode;
 if(row==14)ControllerBridge::PlayerAssists::unlimitedAmmo=!ControllerBridge::PlayerAssists::unlimitedAmmo;
 if(row==15){ControllerBridge::Accessories::unholster=true;ControllerBridge::Accessories::locked=false;ControllerBridge::VRHandsTool::request=-1;}
 if(row==16){if(ControllerBridge::Accessories::page.exchange(0)==1)ControllerBridge::mapRequested=true;GamePointer::manual=false;}
 if(row==17){ControllerBridge::physicalMelee=!ControllerBridge::physicalMelee;SaveSettings();}
 if(row==18){ControllerBridge::widerView=!ControllerBridge::widerView;SaveSettings();}
 if(row==19){++ControllerBridge::Accessories::meleeCycle;ControllerBridge::Accessories::meleeMode=(ControllerBridge::Accessories::meleeMode+1)%3;}
 if(row==20)Hide();dirty=true;
}
bool Hit(vr::VROverlayHandle_t overlay,const glm::mat4& hand,vr::VROverlayIntersectionResults_t& hit){
 float width=overlay==panel?1.6f:1.8f,height=width*float(Height)/Width;
 if(overlay==screen&&gameWindow){RECT r{};if(GetClientRect(gameWindow,&r)&&r.right>0)height=width*float(r.bottom)/r.right;}
 MenuMath::Hit point;if(!MenuMath::Intersect(placement,hand,width,height,point))return false;
 hit.vUVs.v[0]=point.u;hit.vUVs.v[1]=point.v;for(int i=0;i<3;i++)hit.vPoint.v[i]=point.world[i];return true;
}
void Update(const std::array<vr::TrackedDevicePose_t,vr::k_unMaxTrackedDeviceCount>& poses,const glm::mat4& head){
 auto now=GetTickCount64();DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
 vr::VRControllerState_t states[2]{};vr::TrackedDeviceIndex_t ids[2]{};bool valid[2]{};
 for(int i=0;i<2;i++){ids[i]=systemVR->GetTrackedDeviceIndexForControllerRole(i?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand);valid[i]=ids[i]<poses.size()&&poses[ids[i]].bPoseIsValid&&systemVR->GetControllerState(ids[i],&states[i],sizeof(states[i]));}
 bool b=valid[1]&&(states[1].ulButtonPressed&vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu));
 int bAction=bGesture.Update(b,valid[1]&&foreground==GetCurrentProcessId()&&systemVR->IsInputAvailable(),now);
 if(bAction==1){if(ControllerBridge::menuOpen)Hide();else Show(head);}
 if(bAction==2){Hide();if(TabletUI::Active()){int page=ControllerBridge::Accessories::page.load();if(page!=2){ControllerBridge::Accessories::page=page==1?0:1;ControllerBridge::mapRequested=true;}}else{GamePointer::manual=!GamePointer::active.load();ControllerBridge::mapRequested=true;}}
 if(bAction==3){Hide();TabletUI::suspended=true;GamePointer::manual=!GamePointer::active.load();GamePointer::ignoreEscapeUntil=now+300;ControllerBridge::pauseRequested=true;}
 if(!ControllerBridge::menuOpen)return;
 DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
 if(!systemVR->IsInputAvailable()||(screenMode&&pid!=GetCurrentProcessId())){MouseRelease();triggerHeld[0]=triggerHeld[1]=false;if(cursor)vr::VROverlay()->HideOverlay(cursor);return;}
 bool pressed[2]{};for(int i=0;i<2;i++)pressed[i]=valid[i]&&(states[i].rAxis[1].x>.55f||(states[i].ulButtonPressed&vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)));
 int target=-1;vr::VROverlayIntersectionResults_t hits[2]{};bool hitsPanel[2]{};
 for(int i=0;i<2;i++)hitsPanel[i]=valid[i]&&Hit(screenMode?screen:panel,Stereo::Pose(poses[ids[i]].mDeviceToAbsoluteTracking),hits[i]);
 for(int i=0;i<2;i++)if(hitsPanel[i]&&pressed[i]&&!triggerHeld[i])pointerHand=i;
 if(hitsPanel[pointerHand])target=pointerHand;else if(hitsPanel[1-pointerHand]){pointerHand=1-pointerHand;target=pointerHand;}
 if(target>=0&&cursor){
  auto pointer=placement;pointer[3]=glm::vec4(hits[target].vPoint.v[0],hits[target].vPoint.v[1],hits[target].vPoint.v[2],1.f)+placement[2]*.001f;
  auto matrix=Matrix34(pointer);vr::VROverlay()->SetOverlayTransformAbsolute(cursor,vr::TrackingUniverseStanding,&matrix);vr::VROverlay()->ShowOverlay(cursor);
 }else if(cursor)vr::VROverlay()->HideOverlay(cursor);
 bool trigger=target>=0&&pressed[target];bool clicked=target>=0&&trigger&&!triggerHeld[target];
 if(screenMode){
  if(target>=0&&gameWindow){RECT r{};GetClientRect(gameWindow,&r);POINT at{LONG(hits[target].vUVs.v[0]*(r.right-r.left)),LONG(hits[target].vUVs.v[1]*(r.bottom-r.top))};ClientToScreen(gameWindow,&at);
   INPUT in{};in.type=INPUT_MOUSE;in.mi.dwFlags=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK;
   in.mi.dx=LONG((at.x-GetSystemMetrics(SM_XVIRTUALSCREEN))*65535.0/(std::max)(1,GetSystemMetrics(SM_CXVIRTUALSCREEN)-1));in.mi.dy=LONG((at.y-GetSystemMetrics(SM_YVIRTUALSCREEN))*65535.0/(std::max)(1,GetSystemMetrics(SM_CYVIRTUALSCREEN)-1));SendInput(1,&in,sizeof(in));
  }
  if(clicked&&!mouseHeld){INPUT in{};in.type=INPUT_MOUSE;in.mi.dwFlags=MOUSEEVENTF_LEFTDOWN;mouseHeld=SendInput(1,&in,sizeof(in))==1;}else if(!trigger)MouseRelease();
 }else{
  int row=target>=0?MenuMath::Row(hits[target].vUVs.v[0],hits[target].vUVs.v[1]):-1;
  if(row>=0)lastHoverTime=GetTickCount64();
  if(row!=hover&&(row>=0||GetTickCount64()-lastHoverTime>120))hover=row;if(clicked&&row>=0)Click(row);Draw();
 }
 triggerHeld[0]=pressed[0];triggerHeld[1]=pressed[1];
}
}
