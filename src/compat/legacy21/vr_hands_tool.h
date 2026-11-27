#pragma once
namespace VRHandsTool {
std::atomic<int> request{0};void* lastManager=nullptr;ULONGLONG nextRegister=0,nextSelect=0;int pending=0,attempts=0;TagName previous("sledge");
bool Equal(const std::uint8_t* text,const char* value){return Readable(text,32)&&text[31]==0&&std::strncmp(reinterpret_cast<const char*>(text),value,31)==0;}
void Tick(void* player){
 if(player!=LocalPlayer()||!Readable(player,0x998))return;
 auto manager=*reinterpret_cast<std::uint8_t**>(static_cast<std::uint8_t*>(player)+0x990);if(!Readable(manager,0x60))return;
 TagName id("vrhands");int count=*reinterpret_cast<int*>(manager+0x10);auto list=*reinterpret_cast<void***>(manager+0x18);
 if(count<0||count>4096||!Readable(list,count*sizeof(void*)))return;
 bool registered=false;for(int i=0;i<count;i++){auto tool=static_cast<std::uint8_t*>(list[i]);if(Readable(tool,0x38)&&Equal(tool+0x18,"vrhands")){registered=true;break;}}
 if(!registered&&GetTickCount64()>=nextRegister){
  nextRegister=GetTickCount64()+5000;
  TagName label("VR hands");struct Paths {TagName prefix{""};TagName model{"prefab/tool/spraycan.xml"};} paths;
  using Register=void(*)(void*,TagName*,TagName*,Paths*,int);
  reinterpret_cast<Register>(gameBase+0x441DF0)(manager,&id,&label,&paths,1);
  TagName enableId("vrhands");using Enable=void(*)(void*,TagName*,bool);reinterpret_cast<Enable>(gameBase+0x44B9E0)(manager,&enableId,true);
  Log("Registered VR hands tool");return;
 }
 auto selected=*reinterpret_cast<std::uint8_t**>(manager+0x30);bool isHands=Readable(selected,0x38)&&Equal(selected+0x18,"vrhands");
 if(manager!=lastManager){lastManager=manager;if(handsOnly)request=1;}
 int action=request.exchange(0);if(action){pending=action;attempts=0;nextSelect=0;
  if(action>0&&!isHands&&Readable(selected,0x38)&&selected[0x18+31]==0)std::memcpy(&previous,selected+0x18,32);
 }
 if(pending&&attempts&&isHands==(pending>0)){Log(std::string("VR hands selection confirmed after game update: ")+(isHands?"ON":"OFF"));pending=0;}
 if(pending&&registered&&GetTickCount64()>=nextSelect){
  nextSelect=GetTickCount64()+150;TagName selection=pending>0?id:previous;using Select=void(*)(void*,TagName*,bool,void*);
  TagName enableId("vrhands");using Enable=void(*)(void*,TagName*,bool);reinterpret_cast<Enable>(gameBase+0x44B9E0)(manager,&enableId,true);
  reinterpret_cast<Select>(gameBase+0x44B5B0)(manager,&selection,false,nullptr);
  selected=*reinterpret_cast<std::uint8_t**>(manager+0x30);isHands=Readable(selected,0x38)&&Equal(selected+0x18,"vrhands");
  if(++attempts>=10){Log("VR hands selection retry limit reached");pending=0;}
 }
 if(!pending)handsOnly=isHands;
}
}
