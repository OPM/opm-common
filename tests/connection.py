import unittest
import sunbeam

class TestWells(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.sch = sunbeam.parse('spe3/SPE3CASE1.DATA').schedule 
        cls.timesteps = cls.sch.timesteps
        cls.wells = cls.sch.wells

    def test_connection_pos(self):
        p00 = self.wells[0].connections(0)[0].pos
        p01 = self.wells[0].connections(0)[1].pos
        p10 = self.wells[1].connections(0)[0].pos
        p11 = self.wells[1].connections(0)[1].pos
        self.assertEqual(p00, (6,6,2))
        self.assertEqual(p01, (6,6,3))
        self.assertEqual(p10, (0,0,0))
        self.assertEqual(p11, (0,0,1))

    def test_connection_state(self):
        for well in self.wells:
            for timestep,_ in enumerate(self.timesteps):
                for connection in well.connections(timestep):
                    self.assertEqual("OPEN", connection.state)

    def test_filters(self):
        flowing = sunbeam.Connection.flowing()
        closed = sunbeam.Connection.closed()
        connections = self.wells[0].connections(0)
        self.assertEqual(len(list(filter(flowing, connections))), 2)
        self.assertEqual(len(list(filter(closed, connections))), 0)

    def test_direction(self):
        for well in self.wells:
            for timestep,_ in enumerate(self.timesteps):
                for connection in well.connections(timestep):
                    self.assertEqual(connection.direction, 'Z')

    def test_attached_to_segment(self):
        for well in self.wells:
            for timestep,_ in enumerate(self.timesteps):
                for connection in well.connections(timestep):
                    self.assertFalse(connection.attached_to_segment)

if __name__ == "__main__":
    unittest.main()
