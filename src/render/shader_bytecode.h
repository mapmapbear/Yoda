#pragma once
#include "rhi/rhi_context_commons.h"
#include <string>
#include <vector>

namespace Yoda {

struct ShaderBytecode {
  ShaderType type;
  std::vector<uint32_t> bytecode;
};

class ShaderCompiler {
public:
  static void CompileShader(const std::string &path,
                            const std::string &entryPoint, ShaderType type,
                            ShaderBytecode &bytecode);
};
} // namespace Yoda
