#pragma once
// ABI verified against QueryRaycast / GetShapeClosestPoint in the pinned build.
namespace NativeContact {
struct RejectList {int count=0,capacity=4;void** data=nullptr;void* local[4]{};RejectList(){data=local;}};
struct Filter {unsigned any=0xffff,all=0,exclude=0x340;unsigned char layer=0xff,transparent=0;unsigned short reserved=0;RejectList bodies,shapes;};
static_assert(offsetof(Filter,bodies)==0x10&&offsetof(Filter,shapes)==0x40);
using RayFn=bool(*)(void*,const glm::vec3*,const glm::vec3*,float,Filter*,float*,glm::vec3*,void**,void*,void*);
using ClosestFn=bool(*)(void*,const glm::vec3*,float,float*,glm::vec3*,glm::vec3*,bool,void*,void*,bool);
bool Closest(void* shape,glm::vec3 from,float reach,glm::vec3& point,glm::vec3& normal){
 if(!Readable(shape,0xF8)||!Readable(*reinterpret_cast<void**>(static_cast<std::uint8_t*>(shape)+0x10),0x60))return false;
 float distance=reach;return reinterpret_cast<ClosestFn>(gameBase+0x4ea410)(shape,&from,reach,&distance,&point,&normal,false,nullptr,nullptr,false)&&std::isfinite(point.x)&&std::isfinite(point.y)&&std::isfinite(point.z);
}
bool Ray(glm::vec3 from,glm::vec3 to,void* ignore,float& distance,glm::vec3& normal,std::uint8_t*& body,bool includeHeld=false){
 auto game=*reinterpret_cast<std::uint8_t**>(gameBase+0x1c96e70);if(!Readable(game,0x58))return false;
 auto scene=*reinterpret_cast<void**>(game+0x50);if(!Readable(scene,0x20))return false;
 auto delta=to-from;float length=glm::length(delta);if(length<.00001f||length>2.f)return false;auto direction=delta/length;
 Filter filter;void* rejected[12]{};filter.bodies.data=rejected;filter.bodies.capacity=12;
 for(auto candidate:{ignore,includeHeld?nullptr:static_cast<void*>(DualGrab::grip[0].body),includeHeld?nullptr:static_cast<void*>(DualGrab::grip[1].body)})if(candidate)rejected[filter.bodies.count++]=candidate;
 static const TagName bone("bone"),accessory("sledge_accessory");
 for(int attempt=0;attempt<8;attempt++){
  void* shape=nullptr;distance=length;
  if(!reinterpret_cast<RayFn>(gameBase+0xf1030)(scene,&from,&direction,length,&filter,&distance,&normal,&shape,nullptr,nullptr))return false;
  if(!Readable(shape,0x18))return false;body=*reinterpret_cast<std::uint8_t**>(static_cast<std::uint8_t*>(shape)+0x10);
  if(!Readable(body,0x148))return false;
  if(!hasTag(body,&bone)&&!hasTag(body,&accessory)){float norm=glm::length(normal);if(!std::isfinite(norm)||norm<.001f)return false;normal/=norm;return std::isfinite(distance)&&distance>=0&&distance<=length;}
  rejected[filter.bodies.count++]=body;
 }
 return false;
}
}
