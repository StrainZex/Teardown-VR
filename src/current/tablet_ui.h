#pragma once
namespace TabletUI {
constexpr int Width=1280,Height=800;
std::atomic<bool> pointing{false};
vr::VROverlayHandle_t keyboardPanel=0;glm::mat4 keyboardPose{1.f};
bool lastHome=false,lastKeyboard=false;std::array<unsigned char,Width*Height*4> pixels{};
bool suspended=false;
bool Active(){return !suspended&&ControllerBridge::Accessories::Alive()&&ControllerBridge::Accessories::tabletHand>=0;}
bool Pose(const std::array<vr::TrackedDevicePose_t,vr::k_unMaxTrackedDeviceCount>& poses,glm::mat4& out){
 int hand=ControllerBridge::Accessories::tabletHand.load();if(hand<0||hand>1)return false;
 auto id=systemVR->GetTrackedDeviceIndexForControllerRole(hand?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand);
 if(id>=poses.size()||!poses[id].bPoseIsValid)return false;
 std::lock_guard<std::mutex> guard(ControllerBridge::Accessories::poseMutex);
 out=Stereo::Pose(poses[id].mDeviceToAbsoluteTracking)*ControllerBridge::Accessories::tabletGrip*glm::translate(glm::mat4(1),glm::vec3(0,0,.01175f));return true;
}
vr::HmdMatrix34_t Matrix(const glm::mat4& m){vr::HmdMatrix34_t r{};for(int y=0;y<3;y++)for(int x=0;x<4;x++)r.m[y][x]=m[x][y];return r;}
void Draw(vr::VROverlayHandle_t panel,bool keyboard){
 HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=Width;info.bmiHeader.biHeight=-Height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
 void* bits=nullptr;auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!dc||!bitmap||!bits){if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);return;}
 auto old=SelectObject(dc,bitmap);RECT all{0,0,Width,Height};auto brush=CreateSolidBrush(RGB(13,24,35));FillRect(dc,&all,brush);DeleteObject(brush);
 auto font=CreateFontW(keyboard?70:85,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");auto oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(240,245,250));
 auto label=[&](RECT box,const wchar_t* text){auto b=CreateSolidBrush(RGB(35,59,80));FillRect(dc,&box,b);DeleteObject(b);DrawTextW(dc,text,-1,&box,DT_CENTER|DT_VCENTER|DT_SINGLELINE);};
 if(!keyboard){label({40,100,1240,340},L"Map");label({40,440,1240,680},L"Spawn menu");}
 else{const wchar_t* rows[]={L"1234567890",L"qwertyuiop",L"asdfghjkl-",L"zxcvbnm.,?"};for(int r=0;r<4;r++)for(int c=0;c<10;c++){wchar_t text[2]={rows[r][c],0};label({c*128+4,r*150+4,(c+1)*128-4,(r+1)*150-4},text);}label({4,610,420,790},L"Space");label({430,610,850,790},L"Backspace");label({860,610,1276,790},L"Enter");}
 GdiFlush();auto p=static_cast<unsigned char*>(bits);for(int i=0;i<Width*Height;i++){std::swap(p[i*4],p[i*4+2]);p[i*4+3]=255;}std::memcpy(pixels.data(),bits,pixels.size());vr::VROverlay()->SetOverlayRaw(panel,pixels.data(),Width,Height,4);
 SelectObject(dc,oldFont);SelectObject(dc,old);DeleteObject(font);DeleteObject(bitmap);DeleteDC(dc);
}
void Key(float u,float v){
 int x=(std::clamp)(int(u*10),0,9),y=(std::clamp)(int(v*800/150),0,4);wchar_t ch=0;WORD key=0;
 const wchar_t* rows[]={L"1234567890",L"qwertyuiop",L"asdfghjkl-",L"zxcvbnm.,?"};
 if(y<4)ch=rows[y][x];else if(u<1.f/3)ch=L' ';else key=u<2.f/3?VK_BACK:VK_RETURN;
 INPUT input[2]{};input[0].type=input[1].type=INPUT_KEYBOARD;
 if(ch){input[0].ki.wScan=input[1].ki.wScan=ch;input[0].ki.dwFlags=KEYEVENTF_UNICODE;input[1].ki.dwFlags=KEYEVENTF_UNICODE|KEYEVENTF_KEYUP;}
 else{input[0].ki.wVk=input[1].ki.wVk=key;input[1].ki.dwFlags=KEYEVENTF_KEYUP;}
 SendInput(2,input,sizeof(INPUT));
 if(key==VK_RETURN){ControllerBridge::Accessories::keyboard=false;ControllerBridge::Accessories::spawnRequest=2;}
}
void Keyboard(const glm::mat4& body,bool show){
 auto overlay=vr::VROverlay();if(!overlay)return;
 if(!show){if(keyboardPanel)overlay->HideOverlay(keyboardPanel);lastKeyboard=false;return;}
 if(!keyboardPanel){std::string name="sledge.tablet.keyboard."+std::to_string(GetCurrentProcessId());if(overlay->CreateOverlay(name.c_str(),"Tablet keyboard",&keyboardPanel)!=vr::VROverlayError_None)return;overlay->SetOverlayWidthInMeters(keyboardPanel,.24f);overlay->SetOverlayTexelAspect(keyboardPanel,800.f/260.f);overlay->SetOverlaySortOrder(keyboardPanel,11);}
 keyboardPose=body*glm::translate(glm::mat4(1),glm::vec3(0,-.04875f,.001f));auto m=Matrix(keyboardPose);overlay->SetOverlayTransformAbsolute(keyboardPanel,vr::TrackingUniverseStanding,&m);if(!lastKeyboard)Draw(keyboardPanel,true);lastKeyboard=true;overlay->ShowOverlay(keyboardPanel);
}
}
