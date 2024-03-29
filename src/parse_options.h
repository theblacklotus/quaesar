#pragma once

#include <string>

struct Options {
    // input file such as .adf, .dms, executable, etc.
    std::string input;
    // kickstart file such as kick.rom, kick31.rom, etc.
    std::string kickstart;
    // serial port path ('/tmp/virtual-serial-port')
    std::string serial_port;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Options parse_options(int argc, char** argv, bool* success);
