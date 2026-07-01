#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <cstdlib>

#if defined(_WIN32)
#include <cstring>
#include <process.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

void exit1(Opm::InputErrorAction action)
{
    const char* deckString =
        "RUNSPEC\n"
        "DIMENS\n"
        "  10 10 10 10 /n"
        "\n";

    Opm::ParseContext parseContext;
    Opm::Parser parser;
    Opm::ErrorGuard errors;

    parseContext.update(Opm::ParseContext::PARSE_EXTRA_DATA, action);
    parser.parseString(deckString, parseContext, errors);
}

// This test checks that the application will exit with status 1 - if that is
// requested; since the boost test framework has registered atexit() handlers
// which will unconditionally fail the complete test in the face of an exit(1) -
// this test is implemented without the BOOST testing framework.

#if !defined(_WIN32)
void test_exit(Opm::InputErrorAction action)
{
    pid_t pid = fork();
    if (pid == 0) {
        exit1(action);
    }

    int wait_status{};
    waitpid(pid, &wait_status, 0);

    if (WIFEXITED(wait_status)) {
        // We *want* the child process to terminate with status exit(1),
        // i.e. if the exit status is 0 we fail the complete test with
        // exit(1).
        if (WEXITSTATUS(wait_status) == 0) {
            std::exit(EXIT_FAILURE);
        }
    }
    else {
        std::exit(EXIT_FAILURE);
    }
}
#endif

} // Anonymous namespace

#if defined(_WIN32)
// Windows has no fork(); re-run this executable as a child process (one per
// action) so the parent can verify the child exits with a non-zero status,
// mirroring the POSIX fork()/waitpid() logic above.
int main(int argc, char** argv)
{
    const Opm::InputErrorAction actions[] = {
        Opm::InputErrorAction::EXIT1,
        Opm::InputErrorAction::DELAYED_EXIT1,
    };
    if (argc > 2 && std::strcmp(argv[1], "--child") == 0) {
        exit1(actions[std::atoi(argv[2])]);
        return 0;  // exit1() is expected to exit(1) before reaching here
    }
    for (int i = 0; i < 2; ++i) {
        const char idx[2] = { static_cast<char>('0' + i), '\0' };
        const intptr_t rc = _spawnl(_P_WAIT, argv[0], argv[0], "--child", idx,
                                    static_cast<const char*>(nullptr));
        if (rc == 0) {
            std::exit(EXIT_FAILURE);  // child did NOT exit(1) as required
        }
    }
}
#else
int main()
{
    test_exit(Opm::InputErrorAction::EXIT1);
    test_exit(Opm::InputErrorAction::DELAYED_EXIT1);
}
#endif
