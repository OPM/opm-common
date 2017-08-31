from os.path import isfile
import json
import libsunbeam as lib
from .properties import EclipseState


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
    return EclipseState(lib.parse_data(deck, _parse_context(actions)))


def parse_deck(deck, **kwargs):
    args = [deck]

    if 'keywords' in kwargs:
        keywords = kwargs['keywords']
        # this might be a single keyword dictionary, in which case we pack it
        # into a list. If it's not a dict we assume it's an iterable and just
        # carry on
        if isinstance(keywords, dict):
            keywords = [keywords]
        json_keywords = map(json.dumps, keywords)
        args.append(json_keywords);
    else:
        args.append([])

    args.append(isfile(deck)) # If the deck is a file, the deck is read from
                              # that file. Otherwise it is assumed to be a
                              # string representation of the the deck.

    if 'actions' in kwargs:
        args.append(_parse_context(kwargs['actions']))

    return lib.parse_deck(*args)
