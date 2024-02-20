#pragma once

#include <ca_essentials/core/logger.h>
#include <ca_essentials/renderer/material.h>
#include <ca_essentials/renderer/light.h>

#include <Eigen/Core>

#include <filesystem>

namespace globals {
    namespace paths {
        // Use these placeholders when defining assets/shaders paths. During initialization, 
        // these placeholders will be replaced by proper paths
        inline std::string BASE_DIR_PLACEHOLDER = "#BASE_DIR#";
        inline std::filesystem::path SHADERS_DIR = paths::BASE_DIR_PLACEHOLDER + "/shaders";
        inline std::filesystem::path ASSETS_DIR = paths::BASE_DIR_PLACEHOLDER + "/assets";

        inline std::filesystem::path font_fn      = paths::ASSETS_DIR / "OpenSans-Regular.ttf";
        inline std::filesystem::path bold_font_fn = paths::ASSETS_DIR / "OpenSans-SemiBold.ttf";
    }

    namespace logging {
        inline constexpr bool LOG_SHADER_SETUP_MESSAGES = false;
        inline constexpr bool SNAPSHOT_FBO_MESSAGES = true;
    }

    namespace render {
        inline Light light;
        inline Material material;

        // Sphere and cylinder rendering parameters
        inline int sphere_num_radial_slices     = 50;
        inline int cylinder_num_radial_slices   = 50;
        inline int cylinder_num_vertical_slices = 10;

        namespace debug {
            // Percentage of the average bbox extension
            inline float sphere_radius = 0.003f;
        }

        namespace edit {
            // Percentage of the average bbox extension
            inline float sphere_radius = 0.01f;
            //inline float sphere_radius = 0.011f;
        }
        namespace selection {
            inline float sphere_radius_perc = edit::sphere_radius * 0.95f;
        }
        namespace model {
            inline float normal_mode_wireframe_edge_width  = 0.1f;
            inline float normal_mode_wireframe_blend_width = 1.0f;

            inline float transparent_mode_wireframe_edge_width = 0.05f;
            inline float transparent_mode_wireframe_blend_width = 0.25f;
        }
        namespace straight_chains {
            inline float cylinder_radius_perc = 0.0008f;
            inline float sphere_radius_perc = 0.00150f;

            inline Eigen::Vector3f endpoint_color(0.4f, 0.4f, 0.4f);
            inline Eigen::Vector3f edge_color(0.98f, 0.63f, 0.06f);
        }
    }
    
    namespace io {
        // percentage of the window height
        inline float crop_width = 1.3f;
        inline int crop_top_margin = 60;

        inline bool export_detailed_opt_info = false;
    }

    namespace ui {
        inline float close_button_width = 20.0f;
        inline float section_opening_margin = 0.1f;
        inline float section_ending_gap = 5.0f;
        inline Eigen::Vector3f crop_frame_color(0.0f, 0.0f, 0.0f);

        // Font information
        inline int font_size = 22;
        inline std::string default_font_id = "defaut_font";
        inline std::string bold_font_id    = "bold_font";

        inline Eigen::Vector3f footer_message_color(0.7f, 0.7f, 0.7f);
    }

    namespace reshaping {
        inline constexpr int default_max_iters = 100;
        inline constexpr double zero_displacement_tol = 1e-6;
        inline constexpr double straight_chain_angle_tol = 3.5 * M_PI / 180.0;
        inline constexpr double planar_regions_angle_tol = 3.0 * M_PI / 180.0;
    }
}
