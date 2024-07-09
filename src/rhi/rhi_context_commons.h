#pragma once
#define MAX_FRAMES_COUNT 3
namespace Yoda {
enum class CommandQueueFamily {
  TYPE_GRAPHICS = 0,
  TYPE_COMPUTE = 2,
  TYPE_COPY = 3
};

class RHIContextCommons {
public:
  int x;
};

} // namespace Yoda
