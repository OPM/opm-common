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

    def group(self, timestep=0):
        return {grp.name: grp for grp in self.groups(timestep)}

    def groups(self, timestep=0):
        return [Group(x, self, timestep) for x in self._groups if x.name != 'FIELD']

    def __getitem__(self,well):
         return Well(self._getwell(well))


@delegate(lib.Well)
class Well(object):

    def pos(self, timestep = None):
        if timestep is None:
            return self.I(), self.J(), self.ref()
        return self.I(timestep), self.J(timestep), self.ref(timestep)

    def __repr__(self):
        return 'Well(name = "%s")' % self.name

    def completions(self, timestep):
        return map(Completion, self._completions(timestep))

    def __eq__(self,other):
        return self._sun.__equal__(other._sun)

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


@delegate(lib.Completion)
class Completion(object):

    @property
    def pos(self):
        return self.I, self.J, self.K

    def __repr__(self):
        return 'Completion(number = {})'.format(self.number)

    # using the names flowing and closed for functions that test if a well is
    # opened or closed at some point, because we might want to use the more
    # imperative words 'open' and 'close' (or 'shut') for *changing* the status
    # later
    @staticmethod
    def flowing():
        def fn(completion): return completion.state == 'OPEN'
        return fn

    @staticmethod
    def closed():
        def fn(completion): return completion.state == 'SHUT'
        return fn

    @staticmethod
    def auto():
        def fn(completion): return completion.state == 'AUTO'
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
        names = self._wellnames(self.timestep)
        return [w for w in self._schedule.wells if w.name in names]

    @property
    def parent(self):
        par = self._schedule._group_tree(self.timestep)._parent(self.name)
        if self.name == 'FIELD':
            return None
        else:
            return Group(self._schedule._group(par), self._schedule, self.timestep)

    @property
    def children(self):
        chl = self._schedule._group_tree(self.timestep)._children(self.name)
        g = lambda elt : Group(self._schedule._group(elt),
                               self._schedule,
                               self.timestep)
        return [g(elem) for elem in chl]
