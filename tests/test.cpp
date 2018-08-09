#define CATCH_CONFIG_RUNNER

#include <string>

#include "catch.hpp"


std::string g_location;


int main(int argc, char* argv[]) {
    Catch::Session session; // There must be exactly one instance

    std::string location;

    // Build a new parser on top of Catch's
    using namespace Catch::clara;
    auto cli = Arg(location, "location") // bind variable to a new option, with a hint string
               ("The location of the dataset to test") |
               session.cli(); // description string for the help output

    // Now pass the new composite back to Catch so it uses that
    session.cli(cli);

    // Let Catch (using Clara) parse the command line
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) // Indicates a command line error
        return returnCode;

    g_location = location;

    return session.run();
}
