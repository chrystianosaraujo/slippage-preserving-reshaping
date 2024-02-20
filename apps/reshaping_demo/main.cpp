/**
 * This demo provides a quick example  usage of the Slippage-Preserving 3D Reshaping tool
 *
 * Author: Chrystiano Araujo (araujoc@cs.ubc.ca)
 *
 *
 * Usage:
 *      reshaping_demo.exe -i <input_mesh.obj> -o <output_folder> -e <edit_label>
 *
 *      If -e <edit_label> is not provided, the available edit operations will be listed.
 *
 * For more information, please refer to the paper and extra information available in the project website below:
 *      https://www.cs.ubc.ca/labs/imager/tr/2023/3DReshaping/
 */
#include "load_input_data.h"

#include <mesh_reshaping/globals.h>
#include <mesh_reshaping/reshaping_params.h>
#include <mesh_reshaping/reshaping_data.h>
#include <mesh_reshaping/reshaping_tool.h>
#include <mesh_reshaping/reshaping_tool_io.h>
#include <mesh_reshaping/precompute_reshaping_data.h>
#include <mesh_reshaping/data_filenames.h>

#include <CLI/CLI.hpp>
#include <filesystem>
#include <string>

struct CLIArgs {
    std::string input_fn;
    std::string edit_label;
    std::string output_dir;
    std::string temp_dir;

    int max_iters  = 100;
    bool handle_error_distrib_on = false;
};

void setup_logger() {
    LOGGER.set_level(spdlog::level::level_enum::info);
}

void setup_directories(CLIArgs& args) {
    namespace fs = std::filesystem;

    if(!fs::is_directory(args.output_dir))
        fs::create_directory(args.output_dir);

    if(args.temp_dir.empty()) {
        fs::path temp_dir = fs::path(args.output_dir) / "debug";
        args.temp_dir = temp_dir.string();

        fs::create_directory(args.temp_dir);
    }
}

int parse_command_args(int argc, char const* argv[], CLIArgs& args) {
    namespace fs = std::filesystem;

    CLI::App cli_app{ argv[0] };

    cli_app.allow_extras(true);
    cli_app.add_option("-i, --input" , args.input_fn     , "Input mesh filename (.obj)")->required();
    cli_app.add_option("-o, --output", args.output_dir   , "Output folder")->required();
    cli_app.add_option("-e, --edit"  , args.edit_label   , "Edit label to be loaded. If no value is provided, "
                                                           "all the available edit operations will be listed.");

    try {
        cli_app.parse((argc), (argv));
        return 0;
    } catch(const CLI::ParseError &e) {
        cli_app.exit(e);
        return 1;
    }
}

void print_input_args(const CLIArgs& cli_args) {
    LOGGER.info("    Input         : {}"     , cli_args.input_fn);
    LOGGER.info("    Out Dir       : {}"     , cli_args.output_dir);
    LOGGER.info("    Temp Dir      : {}"     , cli_args.temp_dir);
    LOGGER.info("    Edit Label    : {}"     , cli_args.edit_label);
    LOGGER.info("    Max Iters     : {}"     , cli_args.max_iters);

    LOGGER.info("");
}

void save_optimization_inputs(const reshaping::TriMesh& mesh,
                              const reshaping::EditOperation& edit_op,
                              const std::string& output_dir,
                              const std::string& run_name) {
    namespace meshes = ca_essentials::meshes;
    namespace fs = std::filesystem;

    // Exporting normalized input  mesh
    if(!reshaping::save_mesh(mesh.get_vertices(),
                             mesh.get_facets(),
                             output_dir,
                             run_name,
                             "input")) {
        LOGGER.error("Error while saving normalized input mesh to {}", output_dir);
    }

    // Exporting handles and fixed points
    if(!reshaping::save_edit_operation_to_obj(edit_op,
                                              mesh.get_vertices(),
                                              mesh.get_facets(),
                                              output_dir,
                                              run_name)) {
        LOGGER.error("Error while exporting edit operation (handles and fixed points) at {}", output_dir);
    }
}

void save_optimization_outputs(const reshaping::ReshapingParams& params,
                               const reshaping::ReshapingData& opt_data,
                               const std::string& output_dir,
                               const std::string& run_name) {
    namespace meshes = ca_essentials::meshes;
    namespace fs = std::filesystem;

    // Saving output mesh
    if(!reshaping::save_mesh(opt_data.mesh,
                             output_dir,
                             run_name,
                             "output")) {
        LOGGER.error("Error while exporting reshaping output mesh at {}", output_dir);
    }

    // Saving optimization info to json
    if(!reshaping::save_optimization_info_to_json(opt_data.mesh,
                                                  opt_data,
                                                  params,
                                                  output_dir,
                                                  run_name)) {
        LOGGER.info("Optimization info exported at {}", output_dir);
    }
}

void list_edit_operations(const std::string& mesh_fn) {
    namespace fs = std::filesystem;

    std::string op_fn = reshaping::get_edit_operation_fn(mesh_fn);

    std::vector<reshaping::EditOperation> edit_ops;
    reshaping::load_edit_operations_from_json(op_fn, edit_ops);

    LOGGER.info("Available edit operations for {}", fs::path(mesh_fn).stem().string());
    for(const auto& op : edit_ops )
        printf("    %s\n", op.label.c_str());
}

int main(const int argc, const char** argv) {
    namespace fs = std::filesystem;

    setup_logger();

    CLIArgs cli_args;
    if(parse_command_args(argc, argv, cli_args) != 0)
        return 1;

    const std::string& mesh_fn = cli_args.input_fn;
    const std::string& edit_label = cli_args.edit_label;
    const std::string run_name = fs::path(mesh_fn).stem().string() + "_" + edit_label;

    LOGGER.info("Slippage-Preserving Reshaping");
    if(cli_args.edit_label.empty()) {
        list_edit_operations(mesh_fn);
        return 0;
    }

    setup_directories(cli_args);
    print_input_args(cli_args);

    // Loading mesh, edit operation, straightness, and curvature values
    InputData in_data = load_input_data(mesh_fn, edit_label);
    if(!in_data.mesh)
        return 1;

    // Saving input mesh and edit operation
    save_optimization_inputs(*in_data.mesh.get(),
                             *in_data.edit_op.get(),
                             cli_args.output_dir,
                             run_name);

    // Setting up reshaping parameters
    reshaping::ReshapingParams params;
    params.debug_folder = cli_args.temp_dir;
    params.input_name   = run_name;
    params.max_iters    = cli_args.max_iters;
    params.handle_error_distrib_enabled = cli_args.handle_error_distrib_on;

    // Precomputing reshaping data
    auto reshaping_data = reshaping::precompute_reshaping_data(
        params,
        *in_data.mesh.get(),
        in_data.PV1,
        in_data.PV2,
        in_data.straight_info.get()
    );

    // Defining hard constraints
    const double diag_len = in_data.mesh->get_bbox().diagonal().norm();

    for(const auto& [vid, disp] : in_data.edit_op->displacements) {
        const Eigen::Vector3d& orig_pos  = in_data.mesh->get_vertices().row(vid);
        const Eigen::Vector3d target_pos = reshaping::displacement_to_abs_position(orig_pos,
                                                                                   disp,
                                                                                   diag_len);
        reshaping_data->bc.insert({ vid, target_pos });
    }

    LOGGER.debug("Starting Reshaping Tool");
    Eigen::MatrixXd newV = reshaping::reshaping_solve(params, *reshaping_data);

    Eigen::MatrixXd V;
    in_data.mesh->export_vertices(V);
    V = newV;
    in_data.mesh->import_vertices(V);

    save_optimization_outputs(params,
                              *reshaping_data,
                              cli_args.output_dir,
                              run_name);

    return 0;
}