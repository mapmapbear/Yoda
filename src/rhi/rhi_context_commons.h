#pragma once
#define MAX_FRAMES_COUNT 3

namespace Yoda {
enum class CommandQueueFamily {
  TYPE_GRAPHICS = 0,
  TYPE_COMPUTE = 2,
  TYPE_COPY = 3
};

template<typename T>
struct ImgData {
	int width;
	int height;
	int comp;
	T* data;
};

class RHIContextCommons {
public:
  int x;
};

} // namespace Yoda
