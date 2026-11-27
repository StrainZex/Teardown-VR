#pragma once
#include <cstdint>
#include <cstring>
// DrawSprite appends 0x58-byte entries to renderer+0x2420. The native
// transparent pass sorts them in place and clears only the count, leaving the
// allocation alive. Re-expose this frame's entries before the second eye,
// letting the native renderer project/depth-test them using that eye's camera.
// Never replay screen pixels or carry the count into the next simulation frame.
namespace StereoSprites {
struct Queue { std::int32_t count,capacity; void* data; };
static_assert(sizeof(Queue)==16);
class Frame {
 Queue saved{};void* header=nullptr;
public:
 template<class Read> Frame(void* renderer,Read readable){
  if(!renderer)return;
  auto at=static_cast<std::uint8_t*>(renderer)+0x2420;
  if(!readable(at,sizeof(Queue)))return;
  Queue q;std::memcpy(&q,at,sizeof(q));
  if(q.count<=0||q.count>32768||q.capacity<q.count||!q.data||!readable(q.data,std::size_t(q.count)*0x58))return;
  header=at;saved=q;
 }
 void BeforeEye(){
  if(!header)return;
  Queue current;std::memcpy(&current,header,sizeof(current));
  // A replacement allocation or newly queued work belongs to the engine.
  if(current.count==0&&current.data==saved.data&&current.capacity>=saved.count)
   std::memcpy(header,&saved.count,sizeof(saved.count));
 }
};
}
