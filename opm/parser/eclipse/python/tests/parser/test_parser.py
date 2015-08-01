from unittest import TestCase

from ert.test import TestAreaContext

from opm.parser import Parser,ParseMode



class ParserTest(TestCase):
    def test_parse(self):
        p = Parser()
        pm = ParseMode()
        with self.assertRaises(IOError):
            p.parseFile("does/not/exist" , pm)

        with TestAreaContext("parse-test"):
            with open("test.DATA", "w") as fileH:
                fileH.write("RUNSPEC\n")
                fileH.write("DIMENS\n")
                fileH.write(" 10 10 10 /\n")
        
            deck = p.parseFile( "test.DATA" , pm)
            self.assertEqual( len(deck) , 2 )


            
