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
    @property
    def wells(self):
        return map(Well, self._wells)

    def group(self, name):
        return Group(self._group(name), self)

@delegate(lib.EclipseState)
class EclipseState(object):
    @property
    def schedule(self):
        return Schedule(self._schedule())

@delegate(lib.Well)
class Well(object):

    def pos(self, timestep = None):
        if timestep is None:
            return self.I(), self.J(), self.ref()
        return self.I(timestep), self.J(timestep), self.ref(timestep)

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

def parse(path, actions = None):
    return EclipseState(lib.parse(path, _parse_context(actions)))
