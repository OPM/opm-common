#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>


void exit1() {
  const char * deckString =
    "RUNSPEC\n"
    "DIMENS\n"
    "  10 10 10 10 /n"
    "\n";

  Opm::ParseContext parseContext;
  Opm::Parser parser;
  Opm::ErrorGuard errors;

  parseContext.update(Opm::ParseContext::PARSE_EXTRA_DATA , Opm::InputError::EXIT1 );
  parser.parseString( deckString , parseContext, errors );
}



/*
  This test checks that the application will exit with status 1 - if that is
  requested; since the boost test framework has registered atexit() handlers
  which will unconditionally fail the complete test in the face of an exit(1) -
  this test is implemented without the BOOST testing framework.
*/

int main() {
    pid_t pid = fork();
    if (pid == 0)
        exit1();

    int wait_status;
    waitpid(pid, &wait_status, 0);

    if (WIFEXITED(wait_status))
        /*
          We *want* the child process to terminate with status exit(1), here we
          capture the exit status of the child process, and then we invert that
          and return the inverted status as the status of the complete test.
        */
        std::exit( !WEXITSTATUS(wait_status) );
    else
        std::exit(EXIT_FAILURE);
}
