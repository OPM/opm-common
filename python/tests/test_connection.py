import unittest
import opm.parser

class TestWells(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.sch = opm.parser.parse('tests/spe3/SPE3CASE1.DATA').schedule 
        cls.timesteps = cls.sch.timesteps

    def test_connection_pos(self):
        wells = self.sch.get_wells(0)
        p00 = wells[0].connections()[0].pos
        p01 = wells[0].connections()[1].pos
        p10 = wells[1].connections()[0].pos
        p11 = wells[1].connections()[1].pos
        self.assertEqual(p00, (6,6,2))
        self.assertEqual(p01, (6,6,3))
        self.assertEqual(p10, (0,0,0))
        self.assertEqual(p11, (0,0,1))

    def test_connection_state(self):
        for timestep,_ in enumerate(self.timesteps):
            for well in self.sch.get_wells(timestep):
                for connection in well.connections():
                    self.assertEqual("OPEN", connection.state)

    def test_filters(self):
        flowing = opm.parser.schedule.Connection.flowing()
        closed = opm.parser.schedule.Connection.closed()
        connections = self.sch.get_wells(0)[0].connections()
        self.assertEqual(len(list(filter(flowing, connections))), 2)
        self.assertEqual(len(list(filter(closed, connections))), 0)

    def test_direction(self):
       for timestep,_ in enumerate(self.timesteps):
           for well in self.sch.get_wells(timestep):
                for connection in well.connections():
                    self.assertEqual(connection.direction, 'Z')

    def test_attached_to_segment(self):
        for timestep,_ in enumerate(self.timesteps):
            for well in self.sch.get_wells(timestep):
                for connection in well.connections():
                    self.assertFalse(connection.attached_to_segment)

if __name__ == "__main__":
    unittest.main()
