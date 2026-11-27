#pragma once
namespace VehicleCamera {
void Request(){
 auto p=static_cast<std::uint8_t*>(LocalPlayer());
 if(!inVehicle||!Readable(p,0x9A0))return;
 vehicleAdjustTarget=*reinterpret_cast<void**>(p+0x998);vehicleAdjustAt=GetTickCount64()+3000;
}
void Tick(void* player,const Snapshot& sample){
 if(!inVehicle)return;auto p=static_cast<std::uint8_t*>(player);auto vehicle=*reinterpret_cast<void**>(p+0x998);auto now=GetTickCount64();
 auto deadline=vehicleAdjustAt.load();if(deadline&&now>=deadline){
  vehicleAdjustAt=0;
  if(vehicle==vehicleAdjustTarget.load()){vehicleHeight=Controllers::SeatHeight(sample.head[3].y);Haptics::Grab(1);}
 }
 static ULONGLONG next=0;if(now<next||menuOpen||HUDMath::interactive)return;next=now+100;
 Transform camera;std::memcpy(&camera,p+0x640,sizeof(camera));if(!Controllers::Valid(camera))return;
 auto head=glm::vec3((BasePose(Controllers::Matrix(camera))*sample.head)[3]);
 auto from=head-glm::vec3(0,.7f,0),to=head+glm::vec3(0,.2f,0);float distance=0;glm::vec3 normal;std::uint8_t* body=nullptr;
 if(!NativeContact::Ray(from,to,nullptr,distance,normal,body)||normal.y>-.3f)return;
 auto ancestor=body;bool ownVehicle=false;
 for(int i=0;i<8&&Readable(ancestor,0x18);i++,ancestor=*reinterpret_cast<std::uint8_t**>(ancestor+0x10))if(ancestor==vehicle){ownVehicle=true;break;}
 if(ownVehicle){float clearance=from.y+distance-head.y-.15f;if(clearance<0)vehicleHeight=glm::clamp(vehicleHeight.load()+clearance,-.65f,.25f);}
}
}
