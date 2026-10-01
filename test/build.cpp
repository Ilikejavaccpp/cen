#include "../vendor/cbp.cpp"

void build_main(int argc, char **argv) {
    CBuildP::file_t files = CBuildP::add_files({
        "snippets/main.c"
    });
    CBuildP::file_t executable = "bin/main";
    CBuildP::include({ "../include" });
    CBuildP::link({ "cen", "sqlite3" }, { "../lib" });
    CBuildP::compile(files, executable);

    /* run... */
    CBuildP::run(executable);
}
