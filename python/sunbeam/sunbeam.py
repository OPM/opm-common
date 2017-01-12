from os.path import isfile
import libsunbeam as lib

class _delegate(object):
    def __init__(self, name, attr):
        self._name = name
        self._attr = attr

    def __get__(self, instance, _):
        if instance is None: return self
        return getattr(self.delegate(instance), self._attr)

    def __set__(self, instance, value):
        setattr(self.delegate(instance), self._attr, value)

    def delegate(self, instance):
        return getattr(instance, self._name)

    def __repr__(self):
        return '_delegate(' + repr(self._name) + ", " + repr(self._attr) + ")"

def delegate(delegate_cls, to = '_sun'):
    attributes = set(delegate_cls.__dict__.keys())

    def inner(cls):
        class _property(object):
            pass

        setattr(cls, to, _property())
        for attr in attributes - set(cls.__dict__.keys() + ['__init__']):
            setattr(cls, attr, _delegate(to, attr))

        def new__new__(_cls, this, *args, **kwargs):
            new = super(cls, _cls).__new__(_cls, *args, **kwargs)
            setattr(new, to, this)  # self._sun = this
            return new

        cls.__new__ = staticmethod(new__new__)

        return cls

    return inner

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

@delegate(lib.Eclipse3DProperties)
class Eclipse3DProperties(object):

    def __repr__(self):
        return 'Eclipse3DProperties()'

@delegate(lib.EclipseState)
class EclipseState(object):
    def __repr__(self):
        return 'EclipseState(title = "%s")' % self.title

    def getNX(self):
        return self._getXYZ()[0]
    def getNY(self):
        return self._getXYZ()[1]
    def getNZ(self):
        return self._getXYZ()[2]

    @property
    def schedule(self):
        return Schedule(self._schedule())

    def props(self):
        return Eclipse3DProperties(self._props())

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


def _parse_context(actions):
    ctx = lib.ParseContext()

    if actions is None:
        return ctx

    # this might be a single tuple, in which case we unpack it and repack it
    # into a list. If it's not a tuple we assume it's an iterable and just
    # carry on
    try:
        key, action = actions
    except ValueError:
        pass
    else:
        actions = [(key, action)]

    for key, action in actions:
        ctx.update(key, action)

    return ctx

def parse(deck, actions = None):
    """deck may be a deck string, or a file path"""
    if isfile(deck):
        return EclipseState(lib.parse(deck, _parse_context(actions)))
    return EclipseState(lib.parseData(deck, _parse_context(actions)))
