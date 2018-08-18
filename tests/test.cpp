#define CATCH_CONFIG_RUNNER

#include <string>

#include "catch.hpp"
#include "config.hpp"


std::string name;
bool use_hl;
bool profile;
bool ranked;


int main(int argc, char* argv[]) {
    Catch::Session session; // There must be exactly one instance

    std::string _name;

    // Build a new parser on top of Catch's
    using namespace Catch::clara;
    auto cli = Arg(_name, "location") // bind variable to a new option, with a hint string
               ("The location of the dataset to test") |
               session.cli(); // description string for the help output

    // Now pass the new composite back to Catch so it uses that
    session.cli(cli);

    // Let Catch (using Clara) parse the command line
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) // Indicates a command line error
        return returnCode;

    name = _name;

    return session.run();
}
