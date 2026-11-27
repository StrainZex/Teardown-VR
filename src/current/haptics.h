#pragma once
namespace Haptics {
// Producers only enqueue an event. SteamVR calls run on the sampling thread.
std::atomic<unsigned> pending{0};
void Grab(int hand){if(hand>=0&&hand<2)pending.fetch_or(1u<<hand);}
void Flush(const vr::TrackedDeviceIndex_t* devices,const bool* valid,bool input){
 auto events=pending.exchange(0);if(!input)return;
 for(int i=0;i<2;i++)if((events&(1u<<i))&&valid[i])systemVR->TriggerHapticPulse(devices[i],0,1800);
}
}
