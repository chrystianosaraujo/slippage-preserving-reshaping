#include "globals.h"
#include "color_scheme_data.h"
#include "application.h"

#include <mesh_reshaping/globals.h>
#include <ca_essentials/core/logger.h>

#include <CLI/CLI.hpp>

#include <filesystem>
#include <regex>

void setup_active_material_and_light(const std::string& scheme_label) {
    LOGGER.debug("Color-scheme: {}", scheme_label);

    ColorScheme color = label_to_color(scheme_label);
    if(color == ColorScheme::UNKNOWN) {
        color = ColorScheme::BLUE;
        LOGGER.warn("Invalid color-scheme: {}. Default blue is used.", scheme_label);
    }

    globals::render::material = get_colorscheme_material(color);
    globals::render::light    = get_colorscheme_light(color);
}

struct CLIArgs {
    bool transparent_mode = false;
    bool run_reshaping = false;
    bool disp_render_off = false;
    bool straight_render_off = false;
    bool export_detailed_opt_info = false;
    bool handle_error_distrib_on = true;
    bool edit_render_off = false;
    bool wireframe_on = false;
    bool normalize_mesh_off = false;

    std::string app_fn;
    std::string input_fn;
    std::string edit_label;
    std::string camera_label;
    std::string camera_label2;
    std::string camera_fn;
    std::string output_dir;
    std::string temp_dir;
    std::string screenshot_fn;
    std::string model_color = "blue";
    std::string load_out_fn;

    int win_width  = 1980;
    int win_height = 1080;
    int max_iters  = 100;
};

void setup_directories(CLIArgs& args) {
    namespace fs = std::filesystem;

    fs::path app_fn = args.app_fn;

    if(!fs::is_directory(args.output_dir))
        fs::create_directory(args.output_dir);

    if(args.temp_dir.empty()) {
        fs::path temp_dir = fs::path(args.output_dir) / "debug";
        args.temp_dir = temp_dir.string();

        fs::create_directory(args.temp_dir);
    }

    if(!fs::exists(globals::paths::SHADERS_DIR)) {
        globals::paths::SHADERS_DIR = fs::path(".") / globals::paths::SHADERS_DIR.stem();
        globals::paths::ASSETS_DIR  = fs::path(".") / globals::paths::ASSETS_DIR.stem();
    }
}

void setup_paths(const std::filesystem::path& exe_fn) {
    namespace fs = std::filesystem;
    namespace paths = globals::paths;

    auto replace_placeholder = [](fs::path& fn, const std::string& base_dir) {
        fn = fs::path(std::regex_replace(fn.string(), std::regex(paths::BASE_DIR_PLACEHOLDER), base_dir));
    };

    std::string base_dir = exe_fn.parent_path().string();
    replace_placeholder(paths::SHADERS_DIR , base_dir);
    replace_placeholder(paths::ASSETS_DIR  , base_dir);
    replace_placeholder(paths::font_fn     , base_dir);
    replace_placeholder(paths::bold_font_fn, base_dir);
}

void setup_globals(CLIArgs& cli_args) {
    globals::io::export_detailed_opt_info = cli_args.export_detailed_opt_info;
}

void setup_logger() {
    LOGGER.set_level(spdlog::level::level_enum::info);
}

bool check_for_asset_files(const std::filesystem::path& exe_fn) {
    namespace fs = std::filesystem;

    fs::path files_to_check[] = {
        globals::paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.fs",
        globals::paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.gs",
        globals::paths::SHADERS_DIR / "blinn_phong_reflection_model_wire.vs",
        globals::paths::font_fn.string(),
    };

    bool all_found = true;
    for(const fs::path& fn : files_to_check) {
        if(!fs::exists(fn)) {
            LOGGER.error("File not found: {}", fn.string());
            all_found = false;
        }
    }

    if(!all_found) {
        LOGGER.error("");
        LOGGER.error("Some asset and shader files are missing. Please make sure that the \"assets\" and \"shaders\" folders "
                     "are copied into the executable folder below.\n    {}\n", exe_fn.parent_path());
        LOGGER.error("These folders are available at PROJECT/apps/reshaping_app");
        return false;
    }

    return true;
}

int parse_command_args(int argc, char const* argv[], CLIArgs& args) {
    CLI::App cli_app{ argv[0] };

    cli_app.allow_extras(true);
    cli_app.add_option("-i, --input"           , args.input_fn               , "Input mesh filename (.obj)");
    cli_app.add_option("-o, --output"          , args.output_dir             , "Output folder")->required();
    cli_app.add_option("-e, --edit"            , args.edit_label             , "Edit label to be loaded");
    cli_app.add_option("--cam"                 , args.camera_label           , "Camera label to be loaded");
    cli_app.add_option("--cam2"                , args.camera_label2          , "2nd Camera label to be loaded. If provided and screenshot enabled, "
                                                                               "a screenshot will be saved for both --cam and --cam2");
    cli_app.add_option("--cam_fn"              , args.camera_fn              , "Camera file to be loaded");
    cli_app.add_option("--max_iters"           , args.max_iters              , "Maximum number of iterations (default: 100)");
    cli_app.add_option("--screenshot_fn"       , args.screenshot_fn          , "Save screenshot and exit. Output screenshot fn must be provided");
    cli_app.add_flag  ("--reshaping"           , args.run_reshaping          , "Run reshaping after loading the input mesh, camera, and edit");
    cli_app.add_option("--model_color"         , args.model_color            , "Model color (blue, green, creamy, pink)");
    cli_app.add_option("--load_out_fn"         , args.load_out_fn            , "Load a pre-computed reshaping solution");
    cli_app.add_flag  ("--transparent_mode"    , args.transparent_mode       , "Draws surface in transparent mode - highlighting edit");
    cli_app.add_flag  ("--wireframe_on"        , args.wireframe_on           , "Enable wireframe mode");
    cli_app.add_flag  ("--disp_render_off"     , args.disp_render_off        , "Disables displacement vectors rendering (handles)");
    cli_app.add_flag  ("--straight_render_off" , args.straight_render_off    , "Enables straight sections rendering");
    cli_app.add_flag  ("--normalize_input_off" , args.normalize_mesh_off     , "Disables model normalization while loading inputs");
    cli_app.add_flag  ("--handle_error_distrib", args.handle_error_distrib_on, "Enables handle-error distribution");
    cli_app.add_flag  ("--edit_render_off"     , args.edit_render_off        , "Disables edit rendering");
    cli_app.add_flag  ("--detailed_opt_info_on", args.export_detailed_opt_info,
                                                "Exports detailed optimization info output OBjs for every iteration");

    CLI11_PARSE(cli_app, argc, argv);
    args.app_fn = argv[0];

    return 0;
}

void print_input_args(const CLIArgs& cli_args) {
    LOGGER.info("    Input         : {}"     , cli_args.input_fn);
    LOGGER.info("    Out Dir       : {}"     , cli_args.output_dir);
    LOGGER.info("    Temp Dir      : {}"     , cli_args.temp_dir);
    LOGGER.info("    Window        : {} x {}", cli_args.win_width, cli_args.win_height);
    LOGGER.info("    Edit Label    : {}"     , cli_args.edit_label);
    LOGGER.info("    Camera Label  : {}"     , cli_args.camera_label);
    LOGGER.info("    Max Iters     : {}"     , cli_args.max_iters);
    LOGGER.info("    Screenshot On : {}"     , !cli_args.screenshot_fn.empty());
    LOGGER.info("    Screenshot Fn : {}"     , cli_args.screenshot_fn);
    LOGGER.info("    Transparent On: {}"     , cli_args.transparent_mode);
    LOGGER.info("    Output to Load: {}"     , cli_args.load_out_fn);

    LOGGER.info("");
}

int main(int argc, const char** argv) {
    namespace fs = std::filesystem;

    setup_logger();
    setup_paths(fs::path(argv[0]));
    if(!check_for_asset_files(fs::path(argv[0])))
        return 1;

    CLIArgs cli_args;
    if(parse_command_args(argc, argv, cli_args) != 0)
        return 1;

    LOGGER.info("Slippage-Preserving Reshaping");

    print_input_args(cli_args);
    setup_directories(cli_args);
    setup_globals(cli_args);
    setup_active_material_and_light(cli_args.model_color);

    Application app(cli_args.output_dir, cli_args.temp_dir);
    app.init(cli_args.win_width, cli_args.win_height);
    
    app.set_max_iters(cli_args.max_iters);
    app.enable_edit_operation_render(!cli_args.edit_render_off);
    app.enable_handle_error_distribution(cli_args.handle_error_distrib_on);
    app.enable_displacement_render(!cli_args.disp_render_off);
    app.enable_straight_render(!cli_args.straight_render_off);
    
    if(!cli_args.input_fn.empty())
        app.load_model(cli_args.input_fn, !cli_args.normalize_mesh_off);
    
    if(!cli_args.load_out_fn.empty())
        app.load_output_solution(cli_args.load_out_fn);
    
    if(!cli_args.edit_label.empty())
        app.activate_edit_operation(cli_args.edit_label);
    
    if(!cli_args.camera_label.empty()) {
        if(!cli_args.camera_fn.empty())
            app.load_camera(cli_args.camera_fn, cli_args.camera_label);
        else
            app.activate_camera(cli_args.camera_label);
    }
    
    if(cli_args.transparent_mode)
        app.enable_transparent_mode(true);
    else if(cli_args.wireframe_on)
        app.enable_wireframe(true);
    
    if(cli_args.run_reshaping)
        app.perform_reshaping();
    
    if(!cli_args.screenshot_fn.empty()) {
        app.run_for_snapshot_only(cli_args.screenshot_fn, 2);
    
        // Save again in case the second camera was passed
        if(!cli_args.camera_label2.empty()) {
            fs::path orig_fn = fs::path(cli_args.screenshot_fn);
            fs::path new_fn = orig_fn.parent_path() / (orig_fn.stem().string() + "_1" + orig_fn.extension().string());
            app.activate_camera(cli_args.camera_label2);
            app.run_for_snapshot_only(new_fn.string(), 2);
        }
    }
    else
        app.run();

    return 0;
}
