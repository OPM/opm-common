from unittest import TestCase
from ert.test import TestAreaContext

from opm.parser import Parser
from opm.ecl_state import TableManager



class TableManagerTest(TestCase):
    def test_create(self):
        with TestAreaContext("table-manager"):
            p = Parser()
            with open("test.DATA", "w") as fileH:
                fileH.write("""
TABDIMS
 2 /

SWOF
 1 2 3 4
 5 6 7 8 /
 9 10 11 12 /
""")
            
            deck = p.parseFile( "test.DATA")
            table_manager = TableManager( deck )
