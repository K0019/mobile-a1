#pragma once
#include <unordered_map>
#include "interface.h"

/*namespace shader::internals
{
  std::unordered_map<uint32_t, std::string> debugGLSLSourceCode;
  // i dont even care anymore, someone should really put this somewhere else
}*/

inline std::string readShaderFile(const char* fileName)
{
  FILE* file;
  fopen_s(&file, fileName, "r");

  if (!file)
  {
    LOG_WARNING("I/O error. Cannot open shader file {}", fileName);
    return {};
  }

  fseek(file, 0L, SEEK_END);
  const size_t bytesinfile = ftell(file);
  fseek(file, 0L, SEEK_SET);

  char* buffer = (char*)alloca(bytesinfile + 1);
  const size_t bytesread = fread(buffer, 1, bytesinfile, file);
  fclose(file);

  buffer[bytesread] = 0;

  static constexpr unsigned char BOM[] = {0xEF, 0xBB, 0xBF};

  if (bytesread > 3) { if (!memcmp(buffer, BOM, 3)) { memset(buffer, ' ', 3); } }

  std::string code(buffer);

  while (code.find("#include ") != code.npos)
  {
    const auto pos = code.find("#include ");
    const auto p1 = code.find('<', pos);
    const auto p2 = code.find('>', pos);
    if (p1 == code.npos || p2 == code.npos || p2 <= p1)
    {
      LOG_WARNING("Error while loading shader program:{}", code.c_str());
      return {};
    }
    const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
    const std::string include = readShaderFile(name.c_str());
    code.replace(pos, p2 - pos + 1, include.c_str());
  }
  return code;
}

inline vk::ShaderStage vkShaderStageFromFileName(const char* fileName)
{
  if (endsWith(fileName, ".vert")) { return vk::ShaderStage::Vert; }
  if (endsWith(fileName, ".frag")) { return vk::ShaderStage::Frag; }
  if (endsWith(fileName, ".geom")) { return vk::ShaderStage::Geom; }
  if (endsWith(fileName, ".comp")) { return vk::ShaderStage::Comp; }
  if (endsWith(fileName, ".tesc")) { return vk::ShaderStage::Tesc; }
  if (endsWith(fileName, ".tese")) { return vk::ShaderStage::Tese; }
  return vk::ShaderStage::Vert;
}

inline vk::Holder<vk::ShaderModuleHandle> loadShaderModule(vk::IContext& ctx, const char* fileName)
{
  const std::string code = readShaderFile(fileName);
  const vk::ShaderStage stage = vkShaderStageFromFileName(fileName);

  if (code.empty()) { return {}; }

  vk::Result res;

  vk::Holder<vk::ShaderModuleHandle> handle = ctx.createShaderModule({code.c_str(), stage, (std::string("Shader Module: ") + fileName).c_str()}, &res);

  if (!res.isOk()) { return {}; }

  //shader::internals::debugGLSLSourceCode[handle.index()] = code;
  // i dont think i need to save this anywhere

  return handle;
}
