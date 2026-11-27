#pragma once
#include <cstdint>
#include <mutex>
// The renderer must never wait for a preempted input/animation reader.
namespace SnapshotCache {
template<class T> bool Publish(std::mutex& mutex,T& destination,const T& value){
 std::unique_lock<std::mutex> lock(mutex,std::try_to_lock);
 if(!lock)return false;
 destination=value;return true;
}
template<class T> class Reader {
 T cached{};
public:
 bool Read(std::mutex& mutex,const T& source,bool active,std::uint64_t now,T& out){
  {std::unique_lock<std::mutex> lock(mutex,std::try_to_lock);if(lock)cached=source;}
  out=cached;
  return active&&out.stamp&&now>=out.stamp&&now-out.stamp<250;
 }
};
}
