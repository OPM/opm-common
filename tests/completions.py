import unittest
import sunbeam

class TestWells(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.sch = sunbeam.parse('spe3/SPE3CASE1.DATA').schedule 
        cls.timesteps = cls.sch.timesteps
        cls.wells = cls.sch.wells

    def test_completion_pos(self):
        p00 = self.wells[0].completions(0)[0].pos
        p01 = self.wells[0].completions(0)[1].pos
        p10 = self.wells[1].completions(0)[0].pos
        p11 = self.wells[1].completions(0)[1].pos
        self.assertEqual(p00, (6,6,2))
        self.assertEqual(p01, (6,6,3))
        self.assertEqual(p10, (0,0,0))
        self.assertEqual(p11, (0,0,1))

    def test_completion_state(self):
        for well in self.wells:
            for timestep,_ in enumerate(self.timesteps):
                for completion in well.completions(timestep):
                    self.assertEqual("OPEN", completion.state)

    def test_filters(self):
        flowing = sunbeam.Completion.flowing()
        closed = sunbeam.Completion.closed()
        completions = self.wells[0].completions(0)
        self.assertEqual(len(list(filter(flowing, completions))), 2)
        self.assertEqual(len(list(filter(closed, completions))), 0)

    def test_direction(self):
        for well in self.wells:
            for timestep,_ in enumerate(self.timesteps):
                for completion in well.completions(timestep):
                    self.assertEqual(completion.direction, 'Z')

    def test_attached_to_segment(self):
        for well in self.wells:
            for timestep,_ in enumerate(self.timesteps):
                for completion in well.completions(timestep):
                    self.assertFalse(completion.attached_to_segment)

if __name__ == "__main__":
    unittest.main()
