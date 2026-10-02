#include "vendor/cbp.cpp"

void build_main(int argc, char **argv) {
    CBuildP::file_t in_files = CBuildP::add_files({
        "examples/cdictionaries.c"
    });
    CBuildP::file_t out_file = "examples/cdictionaries.exec";
    CBuildP::file_t out_file_cpp = "examples/clists_cpp.exec"; /* CHORE: change later */
    /* This optimize is one time */
    CBuildP::optimize({
        .compiler = CBuildP::compilers::c::clang,
        .level = CBuildP::optimization::max,
        .debug = false
    });
    CBuildP::include({ "include" });
    CBuildP::specs(in_files, out_file);
    /* This is one time */
    CBuildP::link({
        "cen"
    }, {
        "lib"
    });
    CBuildP::compile(in_files, out_file);

    /* since shared libs need the linker to know where they
     * are unless you put them in LD_LIBRARY_PATH */
    CBuildP::run(out_file, "", false);

    in_files = CBuildP::add_files({
        "examples/clists.cpp"
    });
    CBuildP::optimize({
        .compiler = CBuildP::compilers::cxx::clang,
    });
    CBuildP::specs(in_files, out_file_cpp);
    CBuildP::compile(in_files, out_file_cpp);
    CBuildP::run(out_file_cpp, "", false);
}
