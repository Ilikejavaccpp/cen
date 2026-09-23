#include "vendor/cbp.cpp"

void build_main(int argc, char **argv) {
    CBuildP::file_t in_files = CBuildP::add_files({
        "examples/clists.c"
    });
    CBuildP::file_t out_file = "examples/clists.exec";
    CBuildP::optimize({
        .compiler = CBuildP::compilers::c::clang,
        .level = CBuildP::optimization::max,
        .debug = false
    });
    CBuildP::specs(in_files, out_file);
    CBuildP::link({
        "cen"
    }, {
        "build/cen-v2"
    });
    CBuildP::compile(in_files, out_file);

    /* since shared libs need the linker to know where they
     * are unless you put them in LD_LIBRARY_PATH */
    CBuildP::run(out_file, "", false);
}
