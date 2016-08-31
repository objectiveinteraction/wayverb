#pragma once

#include "raytracer/program.h"

#include "common/aligned/vector.h"
#include "common/cl/geometry.h"
#include "common/cl/include.h"
#include "common/geo/geometric.h"

#include "glm/glm.hpp"

class scene_buffers;

namespace raytracer {

aligned::vector<geo::ray> get_rays_from_directions(
        const glm::vec3& source, const aligned::vector<glm::vec3>& directions);
aligned::vector<geo::ray> get_random_rays(const glm::vec3& source, size_t num);

class reflector final {
public:
    reflector(const compute_context& cc,
              const glm::vec3& receiver,
              const aligned::vector<geo::ray>& rays,
              double speed_of_sound);

    aligned::vector<reflection> run_step(const scene_buffers& buffers);

    aligned::vector<ray> get_rays() const;
    aligned::vector<reflection> get_reflections() const;
    aligned::vector<cl_float> get_rng() const;

private:
    using kernel_t = decltype(std::declval<program>().get_kernel());

    compute_context cc;
    cl::CommandQueue queue;
    kernel_t kernel;
    cl_float3 receiver;
    size_t rays;

    cl::Buffer ray_buffer;
    cl::Buffer reflection_buffer;

    cl::Buffer rng_buffer;
};

}  // namespace raytracer
