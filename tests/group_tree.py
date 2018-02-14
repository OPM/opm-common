import unittest
import sunbeam


class TestGroupTree(unittest.TestCase):
    def setUp(self):
        norne = '../../examples/data/norne/NORNE_ATW2013.DATA'
        self.sch = sunbeam.parse(norne, [('PARSE_RANDOM_SLASH', sunbeam.action.ignore)]).schedule

    def test_group(self):
        gr = self.sch.group(timestep=2)['PROD']

        self.assertEqual('PROD', gr.name)
        self.assertEqual('MANI-B1', gr.children[0].name)
        self.assertEqual(6, len(gr.children))
        self.assertEqual('FIELD', gr.parent.name)
        self.assertEqual(2, gr.timestep)
        self.assertEqual(None, gr.parent.parent)

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
        self.assertEqual(sunbeam.schedule.Group, type(child))
        self.assertEqual(set(children), set(names))

if __name__ == '__main__':
    unittest.main()
