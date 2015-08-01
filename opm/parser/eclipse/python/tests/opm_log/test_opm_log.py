from unittest import TestCase
from ert.test import TestAreaContext

from opm.log import OpmLog, MessageType


class OpmLogTest(TestCase):
    def test_file_stream(self):
        OpmLog.addStdoutLog( MessageType.ERROR )
        OpmLog.addStderrLog( MessageType.ERROR )

        with TestAreaContext("log/test"):
            OpmLog.addFileLog( "log.txt" , MessageType.INFO )
            OpmLog.addMessage( MessageType.INFO , "XXLine1")

            with open("log.txt") as file:
                line = file.readline()
                line = line.strip()
                self.assertEqual( line , "XXLine1")
                
