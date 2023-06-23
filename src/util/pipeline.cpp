#include "pipeline.hpp"
#include "pipeline/simple.hpp"

namespace util {

using namespace wgpu;

std::vector<RenderPipeline> CreatePipelines(util::Handle &handle) {
  std::vector<RenderPipeline> pipelines{
    CreatePipelineSimple(handle)
  };
  return pipelines;
}

} // namespace util
