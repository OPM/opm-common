import libsunbeam as lib

class EclipseState(lib.EclipseState):
    pass

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
    return lib.parse(path, _parse_context(actions))
