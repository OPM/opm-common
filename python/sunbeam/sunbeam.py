from __future__ import absolute_import


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
        for attr in attributes - set(list(cls.__dict__.keys()) + ['__init__']):
            setattr(cls, attr, _delegate(to, attr))
            src, dst = getattr(delegate_cls, attr), getattr(cls, attr)
            setattr(dst, '__doc__', src.__doc__)

        def new__new__(_cls, this, *args, **kwargs):
            new = super(cls, _cls).__new__(_cls)
            setattr(new, to, this)  # self._sun = this
            return new

        cls.__new__ = staticmethod(new__new__)

        return cls

    return inner
