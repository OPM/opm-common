import libsunbeam as lib
from .sunbeam import delegate

@delegate(lib.Schedule)
class Schedule(object):

    def __repr__(self):
        lt = len(self.timesteps)
        lw = len(self.wells)
        return 'Schedule(timesteps: %d, wells: %d)' % (lt, lw)

    @property
    def wells(self):
        return map(Well, self._wells)

    def group(self, name):
        return Group(self._group(name), self)

    @property
    def groups(self):
        def mk(x): return Group(x, self)
        def not_field(x): return x.name != 'FIELD'
        return map(mk, filter(not_field, self._groups))


@delegate(lib.Well)
class Well(object):

    def pos(self, timestep = None):
        if timestep is None:
            return self.I(), self.J(), self.ref()
        return self.I(timestep), self.J(timestep), self.ref(timestep)

    def __repr__(self):
        return 'Well(name = "%s")' % self.name

    @staticmethod
    def defined(timestep):
        def fn(well): return well.isdefined(timestep)
        return fn

    @staticmethod
    def injector(timestep):
        def fn(well): return well.isinjector(timestep)
        return fn

    @staticmethod
    def producer(timestep):
        def fn(well): return well.isproducer(timestep)
        return fn

    # using the names flowing and closed for functions that test if a well is
    # opened or closed at some point, because we might want to use the more
    # imperative words 'open' and 'close' (or 'shut') for *changing* the status
    # later
    @staticmethod
    def flowing(timestep):
        def fn(well): return well.status(timestep) == 'OPEN'
        return fn

    @staticmethod
    def closed(timestep):
        def fn(well): return well.status(timestep) == 'SHUT'
        return fn

    @staticmethod
    def auto(timestep):
        def fn(well): return well.status(timestep) == 'AUTO'
        return fn

@delegate(lib.Group)
class Group(object):
    def __init__(self, _, schedule):
        self._schedule = schedule

    def wells(self, timestep):
        names = self._wellnames(timestep)
        wells = { well.name: well for well in self._schedule.wells }
        return map(wells.__getitem__, filter(wells.__contains__, names))
