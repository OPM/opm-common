from __future__ import absolute_import

from opm import libopmcommon_python as lib
from .sunbeam import delegate

@delegate(lib.Schedule)
class Schedule(object):

    def __repr__(self):
        lt = len(self.timesteps)
        lw = len(self.wells)
        return 'Schedule(timesteps: %d, wells: %d)' % (lt, lw)

    def get_wells(self, timestep = 0):
        return list(map(Well, self._get_wells(timestep)))

    def group(self, timestep=0):
        return {grp.name: grp for grp in self.groups(timestep)}

    def groups(self, timestep=0):
        return [Group(x, self, timestep) for x in self._groups(timestep) if x.name != 'FIELD']

    def get_well(self, well, timestep):
         return Well(self._getwell(well, timestep))


@delegate(lib.Well)
class Well(object):

    def pos(self):
        return self.I(), self.J(), self.ref()

    def __repr__(self):
        return 'Well(name = "%s")' % self.name

    def connections(self):
        return list(map(Connection, self._connections()))

    def __eq__(self,other):
        return self._sun.__equal__(other._sun)

    @staticmethod
    def defined(timestep):
        def fn(well): return well.isdefined(timestep)
        return fn

    @staticmethod
    def injector():
        def fn(well): return well.isinjector()
        return fn

    @staticmethod
    def producer():
        def fn(well): return well.isproducer()
        return fn

    # using the names flowing and closed for functions that test if a well is
    # opened or closed at some point, because we might want to use the more
    # imperative words 'open' and 'close' (or 'shut') for *changing* the status
    # later
    @staticmethod
    def flowing():
        def fn(well): return well.status() == 'OPEN'
        return fn

    @staticmethod
    def closed():
        def fn(well): return well.status() == 'SHUT'
        return fn

    @staticmethod
    def auto(timestep):
        def fn(well): return well.status(timestep) == 'AUTO'
        return fn


@delegate(lib.Connection)
class Connection(object):

    @property
    def pos(self):
        return self.I, self.J, self.K

    def __repr__(self):
        return 'Connection(number = {})'.format(self.number)

    # using the names flowing and closed for functions that test if a well is
    # opened or closed at some point, because we might want to use the more
    # imperative words 'open' and 'close' (or 'shut') for *changing* the status
    # later
    @staticmethod
    def flowing():
        def fn(connection): return connection.state == 'OPEN'
        return fn

    @staticmethod
    def closed():
        def fn(connection): return connection.state == 'SHUT'
        return fn

    @staticmethod
    def auto():
        def fn(connection): return connection.state == 'AUTO'
        return fn


@delegate(lib.Group)
class Group(object):
    def __init__(self, _, schedule, timestep):

        try:
            if not timestep == int(timestep):
                raise ValueError
        except ValueError:
            raise ValueError('timestep must be int, not {}'.format(type(timestep)))

        if not 0 <= timestep < len(schedule.timesteps):
            raise IndexError('Timestep out of range')

        self._schedule = schedule
        self.timestep = timestep

    def __getitem__(self, name):
        return Group(self._schedule._group(name), self._schedule, self.timestep)

    @property
    def wells(self):
        names = self._wellnames()
        return [w for w in self._schedule.get_wells(self.timestep) if w.name in names]

    @property
    def vfp_table_nr(self):
        vfp_table_nr = self._vfp_table_nr()
        return vfp_table_nr

    @property
    def parent(self):
        if self.name == 'FIELD':
            return None
        else:
            return Group(self._schedule._group(par.name, self.timestep), self._schedule, self.timestep) 

    @property
    def children(self):
        chl = self._schedule._group_tree(self.name, self.timestep)._children()
        g = lambda elt : Group(self._schedule._group(elt.name(), self.timestep),
                               self._schedule,
                               self.timestep)
        return [g(elem) for elem in chl]
