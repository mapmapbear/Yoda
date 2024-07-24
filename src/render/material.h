#pragma once

#include "render/world.h"
#include <glm/glm.hpp>
#include <memory>
#include <nvrhi/nvrhi.h>

namespace Yoda {

class TextureData {

public:
  TextureData() {}

public:
  void *raw_texture_data;
  nvrhi::TextureDesc texture_desc;
  int texture_file_channel;
};

class Material {
public:
  Material(){};
  nvrhi::TextureHandle parse_texture(std::string &path);

public:
  std::string material_name;
  std::vector<std::shared_ptr<TextureData>> texture_dataes;
  glm::vec4 base_color;
  // nvrhi::SamplerHandle main_sampler;
};
} // namespace Yoda
