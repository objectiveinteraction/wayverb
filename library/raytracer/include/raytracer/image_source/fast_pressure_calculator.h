#pragma once

#include "raytracer/cl/structs.h"

#include "common/conversions.h"
#include "common/scene_data.h"
#include "common/surfaces.h"
#include "common/unit_constructor.h"

#include "utilities/aligned/vector.h"
#include "utilities/map_to_vector.h"

/// The image source finder is designed just to ascertain whether or not each
/// 'possible' image-source path is an *actual* image-source path.
/// Once it's worked this out, it delegates to one of these secondary functions
/// to actually compute the pressure or intensity contributed by that particular
/// path.

namespace raytracer {
namespace image_source {

struct reflection_metadata final {
    cl_uint surface_index;  /// The index of the triangle that was intersected.
    float cos_angle;  /// The cosine of the angle against the triangle normal.
};

////////////////////////////////////////////////////////////////////////////////

template <typename Channels, typename It>
auto compute_fast_pressure(const glm::vec3& receiver,
                           const aligned::vector<Channels>& impedance,
                           const aligned::vector<Channels>& scattering,
                           bool flip_phase,
                           const glm::vec3& image_source,
                           It begin,
                           It end) {
    constexpr auto channels = detail::components_v<Channels>;

    const auto surface_attenuation = std::accumulate(
            begin,
            end,
            unit_constructor_v<
                    detail::cl_vector_constructor_t<float, channels>>,
            [&](auto i, auto j) {
                const auto surface_impedance = impedance[j.surface_index];
                const auto surface_scattering = scattering[j.surface_index];

                const auto reflectance =
                        average_wall_impedance_to_pressure_reflectance(
                                surface_impedance, j.cos_angle);
                const auto outgoing =
                        specular_pressure(i * reflectance, surface_scattering);

                return outgoing * (flip_phase ? -1 : 1);
            });

    return impulse<channels>{surface_attenuation,
                             to_cl_float3(image_source),
                             glm::distance(image_source, receiver)};
}

/// A simple pressure calculator which accumulates pressure responses in
/// the time domain.
/// Uses equation 9.22 from the kuttruff book, assuming single-sample
/// reflection/convolution kernels.

template <typename multichannel_type>
class fast_pressure_calculator final {
public:
    template <typename It>
    fast_pressure_calculator(It b_surfaces,
                             It e_surfaces,
                             const glm::vec3& receiver,
                             bool flip_phase)
            : receiver_{receiver}
            , surface_impedances_{map_to_vector(
                      b_surfaces,
                      e_surfaces,
                      [](const auto& surface) {
                          return pressure_reflectance_to_average_wall_impedance(
                                  absorption_to_pressure_reflectance(
                                          surface.absorption));
                      })}
            , surface_scattering_{map_to_vector(
                      b_surfaces,
                      e_surfaces,
                      [](const auto& surface) { return surface.scattering; })}
            , flip_phase_{flip_phase} {}

    using return_type = decltype(compute_fast_pressure(
            std::declval<glm::vec3>(),
            std::declval<aligned::vector<multichannel_type>>(),
            std::declval<aligned::vector<multichannel_type>>(),
            std::declval<bool>(),
            std::declval<glm::vec3>(),
            std::declval<const reflection_metadata*>(),
            std::declval<const reflection_metadata*>()));

    template <typename It>
    return_type operator()(const glm::vec3& image_source,
                           It begin,
                           It end) const {
        return compute_fast_pressure(receiver_,
                                     surface_impedances_,
                                     surface_scattering_,
                                     flip_phase_,
                                     image_source,
                                     begin,
                                     end);
    }

private:
    glm::vec3 receiver_;
    aligned::vector<multichannel_type> surface_impedances_;
    aligned::vector<multichannel_type> surface_scattering_;
    bool flip_phase_;
};

template <typename It>
constexpr auto make_fast_pressure_calculator(It b_surfaces,
                                             It e_surfaces,
                                             const glm::vec3& receiver,
                                             bool flip_phase) {
    using multichannel_type =
            std::decay_t<decltype(std::declval<It>()->absorption)>;
    return fast_pressure_calculator<multichannel_type>{
            std::move(b_surfaces), std::move(e_surfaces), receiver, flip_phase};
}

}  // namespace image_source
}  // namespace raytracer
