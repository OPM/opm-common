class MessageType(object):
    # These are hand-copied from LogUtil.hpp
    DEBUG = 1
    INFO = 2
    WARNING = 4
    ERROR = 8
    PROBLEM = 16
    BUG = 16

    
    DEFAULT = DEBUG + INFO + WARNING + ERROR + PROBLEM + BUG
