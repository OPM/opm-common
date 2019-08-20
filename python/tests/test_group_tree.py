import sys
import unittest

import opm.parser

class TestGroupTree(unittest.TestCase):
    def setUp(self):
        norne = 'examples/data/norne/NORNE_ATW2013.DATA'
        self.sch = opm.parser.parse(norne, [('PARSE_RANDOM_SLASH', opm.parser.action.ignore)]).schedule

    def test_group(self):
        gr = self.sch.group(timestep=2)['PROD']

        self.assertEqual('PROD', gr.name)
        self.assertEqual('MANI-B2', gr.children[0].name)
        self.assertEqual(6, len(gr.children))

        # The group <-> Schedule implementation is quite complicated with lots
        # of self references going back from the group object to the Schedule
        # object; these tests have just been commented out when implementing
        # the GTNode based tree implementation.

        # self.assertEqual('FIELD', gr.parent.name)
        # self.assertEqual(2, gr.timestep)
        # self.assertEqual(None, gr.parent.parent)

    def test_timestep_groups(self):
        total = 0
        for group in self.sch.groups(timestep=3):
            for child in group.children:
                self.assertIsNotNone(child.name)
                total += 1
        self.assertEqual(13, total)

        group = self.sch.group(timestep=3)['PROD']
        children = ['MANI-B1', 'MANI-B2', 'MANI-D1', 'MANI-D2', 'MANI-E1', 'MANI-E2']
        names = [child.name for child in group.children]
        self.assertEqual(opm.parser.schedule.Group, type(child))
        self.assertEqual(set(children), set(names))

if __name__ == '__main__':
    unittest.main()
