#include "vendor/cbp.cpp"

void build_main(int argc, char *argv[]) {
    /* ensure that there is a debug flag to enable
     * object files */
    static bool debug__enable_artifacts = false;

    if (argc > 2 && strcmp(argv[1], "--debug") == 0) {
        if (strcmp(argv[2], "artifacts") == 0) {
            debug__enable_artifacts = true;
        }
    }

    std::vector<CBuildP::file_t> in = {
        "src/stden.c",
    };

    CBuildP::file_t inputfiles = CBuildP::add_files(in);
    CBuildP::file_t out_libfile = "cen"; /* change for building (i.e. build release) */
    CBuildP::file_t out_libdir = "build/cen-v2"; /* change for building (i.e. build release) */

    CBuildP::optimize({
       .compiler = CBuildP::compilers::c::clang,
       .level = CBuildP::optimization::max,
       .debug = true
    });

    CBuildP::specs(inputfiles, out_libfile);

    CBuildP::compile_lib(in, out_libfile, out_libdir, CBuildP::libtype::STATIC, !debug__enable_artifacts);
}
