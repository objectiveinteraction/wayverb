#include "compressed_waveguide.h"

#include "common/progress_bar.h"

#include "waveguide/make_transparent.h"
#include "waveguide/rectangular_waveguide.h"

#include "gtest/gtest.h"

#include <cassert>
#include <cmath>

namespace {
auto uniform_surface(float r) {
    return Surface{VolumeType{{r, r, r, r, r, r, r, r}},
                   VolumeType{{r, r, r, r, r, r, r, r}}};
}

template <typename T>
void multitest(T&& run) {
    constexpr auto iterations = 100;
    const auto proper_output = run();
    for (auto i = 0; i != iterations; ++i) {
        const auto output = run();
        ASSERT_EQ(output, proper_output);
    }
}
}  // namespace

TEST(verify_compensation_signal, verify_compensation_signal_compressed) {
    const aligned::vector<float> input{1, 2, 3, 4, 5, 4, 3, 2, 1};
    const auto transparent = make_transparent(input);

    compute_context c;
    compressed_rectangular_waveguide waveguide(
            c.get_context(), c.get_device(), 100);

    multitest([&] {
        auto t = transparent;
        return waveguide.run_soft_source(std::move(t));
    });
}

TEST(verify_compensation_signal, verify_compensation_signal_normal) {
    const aligned::vector<float> input{1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1};
    const auto transparent = make_transparent(input);

    compute_context cc;
    CuboidBoundary cuboid_boundary(glm::vec3(-1), glm::vec3(1));

    auto scene_data = cuboid_boundary.get_scene_data();
    scene_data.set_surfaces(uniform_surface(0.5));
    MeshBoundary boundary(scene_data);

    constexpr glm::vec3 centre{0, 0, 0};

    rectangular_waveguide waveguide(
            cc.get_context(), cc.get_device(), boundary, centre, 20000);

    auto receiver_index = waveguide.get_index_for_coordinate(centre);

    multitest([&] {
        constexpr auto steps = 100;
        std::atomic_bool keep_going{true};
        progress_bar pb(std::cout, steps);
        const auto output = waveguide.init_and_run(
                centre, transparent, receiver_index, steps, keep_going, [&pb] {
                    pb += 1;
                });

        aligned::vector<float> pressures;
        pressures.reserve(output.size());
        for (const auto& i : output) {
            pressures.push_back(i.get_pressure());
        }

        return pressures;
    });
}