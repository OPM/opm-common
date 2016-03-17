#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/StreamLog.hpp>

extern "C" {

    void add_stdout_log(int64_t messageMask) {
        std::shared_ptr<Opm::StreamLog> streamLog = std::make_shared<Opm::StreamLog>(std::cout , messageMask);
        Opm::OpmLog::addBackend( "STDOUT" , streamLog );
    }

    void add_stderr_log(int64_t messageMask) {
        std::shared_ptr<Opm::StreamLog> streamLog = std::make_shared<Opm::StreamLog>(std::cerr , messageMask);
        Opm::OpmLog::addBackend( "STDERR" , streamLog );
    }

    void add_file_log( const char * filename , int64_t messageMask) {
        std::shared_ptr<Opm::StreamLog> streamLog = std::make_shared<Opm::StreamLog>(filename , messageMask);
        Opm::OpmLog::addBackend( filename , streamLog );
    }

    void log_add_message( int64_t messageType , const char * message) {
        Opm::OpmLog::addMessage(messageType , message );
    }

}
