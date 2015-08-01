from ert.cwrap import BaseCClass, CWrapper
from opm import OPMPrototype

from opm.log import MessageType

class OpmLog(BaseCClass):
    _add_file_log   = OPMPrototype("void add_file_log( char* , int64 )")
    _add_stderr_log = OPMPrototype("void add_stderr_log( int64 )")
    _add_stdout_log = OPMPrototype("void add_stdout_log( int64 )")
    _add_message    = OPMPrototype("void log_add_message( int64 , char* )")

    def __init__(self):
        raise Exception("Can not instantiate - just use class methods")

    @classmethod
    def addStdoutLog(cls , message_mask = MessageType.DEFAULT):
        cls._add_stdout_log(message_mask)


    @classmethod
    def addStderrLog(cls , message_mask = MessageType.DEFAULT):
        cls._add_stderr_log(message_mask)


    @classmethod
    def addFileLog(cls , filename , message_mask = MessageType.DEFAULT):
        cls._add_file_log(filename , message_mask)

        
    @classmethod
    def addMessage(cls , message_type , message):
        cls._add_message(message_type , message)
    

