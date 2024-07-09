#pragma once
#include <nvrhi/nvrhi.h>
namespace Yoda {
class ShaderByteCode {
public:
  ShaderByteCode() : m_type(nvrhi::ShaderType::Pixel), m_byte_code({}){};
  ~ShaderByteCode() = default;
  nvrhi::ShaderType m_type;
  std::vector<uint8_t> m_byte_code;

  void compile_shader(nvrhi::ShaderType type, std::string &path,
                      std::string &entry_point);
};
} // namespace Yoda
