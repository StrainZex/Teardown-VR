#pragma once
namespace HandsOnlyVisual {
struct Hidden {int id;void* entity;};std::vector<Hidden> hidden;void* lastTool=nullptr;
void Update(void* player){
 if(player!=LocalPlayer()||!Readable(player,0x828))return;
 DualGrab::Entities entities;if(!DualGrab::Scene(entities))return;
 static const TagName invisible("invisible"),empty("");
 auto tool=*reinterpret_cast<std::uint8_t**>(static_cast<std::uint8_t*>(player)+0x820);
 bool active=enabled&&handsOnly&&!inVehicle;
 if(!active||tool!=lastTool){
  for(auto& h:hidden)if(entities.Get(h.id)==h.entity&&Readable(h.entity,0x18))originalRemoveTag(h.entity,&invisible);
  hidden.clear();lastTool=tool;
 }
 if(!active||!Readable(tool,0x30))return;
 SupportGrip::active=false;
 auto child=*reinterpret_cast<std::uint8_t**>(tool+0x28);
 for(int n=0;n<1024&&Readable(child,0x30);n++,child=*reinterpret_cast<std::uint8_t**>(child+0x18)){
  if(*reinterpret_cast<unsigned short*>(child+8)!=2)continue;
  int id=*reinterpret_cast<int*>(child+0xC);bool ours=false;for(auto& h:hidden)if(h.id==id&&h.entity==child){ours=true;break;}
  if(!ours&&!hasTag(child,&invisible)){hidden.push_back({id,child});ours=true;}
  if(ours)setTag(child,&invisible,&empty);
 }
}
}
