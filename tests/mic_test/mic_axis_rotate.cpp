//  project internal
#include "waveguide.h"
#include "waveguide_config.h"
#include "scene_data.h"
#include "test_flag.h"
#include "conversions.h"
#include "microphone.h"

#include "raytracer.h"

#include "cl_common.h"

//  dependency
#include "filters_common.h"
#include "sinc.h"
#include "write_audio_file.h"

#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"

#include "sndfile.hh"
#include "samplerate.h"

#include <gflags/gflags.h>

//  stdlib
#include <random>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <map>

enum class PolarPattern {
    omni,
    cardioid,
    bidirectional,
};

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (argc != 2) {
        LOG(INFO) << "expecting an output folder";

        LOG(INFO) << "actually found: ";
        for (auto i = 0u; i != argc; ++i) {
            LOG(INFO) << "arg " << i << ": " << argv[i];
        }

        return EXIT_FAILURE;
    }

    std::string output_folder = argv[1];

    //  global params
    auto config = WaveguideConfig();
    config.get_filter_frequency() = 11025;
    config.get_oversample_ratio() = 1;

    auto context = get_context();
    auto device = get_device(context);
    cl::CommandQueue queue(context, device);

    cl_float3 source{{-0.007, 0.006, 0.006}};
    cl_float3 mic{{-0.007, -1.1, 0.006}};
    auto test_locations = 36;

    try {
        CuboidBoundary boundary(Vec3f(-2.05, -2.5, -1.05),
                                Vec3f(2.05, 2.5, 1.05));
        auto waveguide_program =
            get_program<TetrahedralProgram>(context, device);
        TetrahedralWaveguide waveguide(waveguide_program,
                                       queue,
                                       boundary,
                                       config.get_divisions(),
                                       to_vec3f(mic));
        auto mic_index = waveguide.get_index_for_coordinate(to_vec3f(mic));
        auto source_index =
            waveguide.get_index_for_coordinate(to_vec3f(source));

        auto corrected_source =
            waveguide.get_coordinate_for_index(source_index);
        auto corrected_mic = waveguide.get_coordinate_for_index(mic_index);

        std::cout << "source pos: " << corrected_source << std::endl;
        std::cout << "mic pos: " << corrected_mic << std::endl;

        //       auto steps = 4410;
        auto steps = 200;

        ProgressBar pb(std::cout, steps);
        auto w_results =
            waveguide.run_gaussian(corrected_source,
                                   mic_index,
                                   steps,
                                   config.get_waveguide_sample_rate(),
                                   [&pb] { pb += 1; });

        auto amp_factor = 4e3;

        for (std::string polar_string : {"omni", "cardioid", "bidirectional"}) {
            std::map<std::string, PolarPattern> polar_pattern_map = {
                {"omni", PolarPattern::omni},
                {"cardioid", PolarPattern::cardioid},
                {"bidirectional", PolarPattern::bidirectional},
            };

            PolarPattern polar_pattern = polar_pattern_map[polar_string];
            std::cout << "polar_pattern: " << polar_string << std::endl;

            auto directionality = 0.0;
            switch (polar_pattern) {
                case PolarPattern::omni:
                    directionality = 0;
                    break;
                case PolarPattern::cardioid:
                    directionality = 0.5;
                    break;
                case PolarPattern::bidirectional:
                    directionality = 1;
                    break;
            }

            std::cout << "directionality: " << directionality << std::endl;

            std::ofstream ofile(polar_string + ".r.energies.txt");

            for (auto i = 0u; i != test_locations; ++i) {
                auto angle = i * M_PI * 2 / test_locations + M_PI;

                Microphone microphone(Vec3f(sin(angle), cos(angle), 0),
                                      directionality);

                auto w_pressures = microphone.process(w_results);

                std::vector<float> out_signal(
                    config.get_output_sample_rate() * w_results.size() /
                    config.get_waveguide_sample_rate());

                SRC_DATA sample_rate_info{
                    w_pressures.data(),
                    out_signal.data(),
                    long(w_results.size()),
                    long(out_signal.size()),
                    0,
                    0,
                    0,
                    config.get_output_sample_rate() /
                        config.get_waveguide_sample_rate()};

                src_simple(&sample_rate_info, SRC_SINC_BEST_QUALITY, 1);

                mul(out_signal, amp_factor);

                auto bands = 7;
                auto min_band = 80;

                auto print_energy = [&ofile](const auto& sig, auto band) {
                    auto band_energy = std::accumulate(
                        sig.begin(),
                        sig.end(),
                        0.0,
                        [](auto a, auto b) { return a + b * b; });

                    auto max_val = std::accumulate(
                        sig.begin(),
                        sig.end(),
                        0.0,
                        [](auto a, auto b) { return std::max(a, fabs(b)); });

                    ofile << " band: " << band << " energy: " << band_energy
                          << " max: " << max_val;
                };

                ofile << "iteration: " << i;

                print_energy(out_signal, "full");

                for (auto i = 0; i != bands; ++i) {
                    auto band = out_signal;

                    LinkwitzRileyBandpass bandpass;
                    bandpass.setParams(pow(2, i) * min_band,
                                       pow(2, i + 1) * min_band,
                                       config.get_output_sample_rate());
                    bandpass.filter(out_signal);

                    print_energy(band, i);
                }

                ofile << std::endl;

                auto output_file =
                    build_string(output_folder, "/", i, ".waveguide.full.wav");

                unsigned long format, depth;

                try {
                    format = get_file_format(output_file);
                    depth = get_file_depth(config.get_bit_depth());
                } catch (const std::runtime_error& e) {
                    LOG(INFO) << "critical runtime error: " << e.what();
                    return EXIT_FAILURE;
                }

                LinkwitzRileyLopass lopass;
                lopass.setParams(config.get_filter_frequency(),
                                 config.get_output_sample_rate());
                lopass.filter(out_signal);

                write_sndfile(output_file,
                              {out_signal},
                              config.get_output_sample_rate(),
                              depth,
                              format);
            }
        }
    } catch (const cl::Error& e) {
        LOG(INFO) << "critical cl error: " << e.what();
        return EXIT_FAILURE;
    } catch (const std::runtime_error& e) {
        LOG(INFO) << "critical runtime error: " << e.what();
        return EXIT_FAILURE;
    } catch (...) {
        LOG(INFO) << "unknown error";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}