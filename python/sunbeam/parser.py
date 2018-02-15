from __future__ import absolute_import
from os.path import isfile
import json

from sunbeam import libsunbeam as lib
from .properties import EclipseState


def _parse_context(recovery):
    ctx = lib.ParseContext()

    if not recovery:
        return ctx

    # this might be a single tuple, in which case we unpack it and repack it
    # into a list. If it's not a tuple we assume it's an iterable and just
    # carry on
    if not isinstance(recovery, list):
        recovery = [recovery]

    for key, action in recovery:
        ctx.update(key, action)

    return ctx


def parse(deck, recovery=[]):
    """Parse a deck from either a string or file.

    Args:
        deck (str): Either an eclipse deck string or path to a file to open.
        recovery ((str, action)|[(str, action)]): List of error recoveries.
            An error recovery is defined by a pair of a string naming the error
            event to be handled and the action taken for this error event.
            The named error event can be one of the following:
                "PARSE_UNKNOWN_KEYWORD"
                "PARSE_RANDOM_TEXT"
                "PARSE_RANDOM_SLASH"
                "PARSE_MISSING_DIMS_KEYWORD"
                "PARSE_EXTRA_DATA"
                "PARSE_MISSING_INCLUDE"
                "UNSUPPORTED_SCHEDULE_GEO_MODIFIER"
                "UNSUPPORTED_COMPORD_TYPE"
                "UNSUPPORTED_INITIAL_THPRES"
                "UNSUPPORTED_TERMINATE_IF_BHP"
                "INTERNAL_ERROR_UNINITIALIZED_THPRES"
                "SUMMARY_UNKNOWN_WELL"
                "SUMMARY_UNKNOWN_GROUP"
            The avaiable recovery actions can be one of the following:
                sunbeam.action.throw
                sunbeam.action.warn
                sunbeam.action.ignore

    Example:
        Parses a EclipseState from the NORNE data set with recovery set to
        ignore PARSE_RANDOM_SLASH error events.
            es = sunbeam.parse('~/opm-data/norne/NORNE_ATW2013.DATA',
                recovery=('PARSE_RANDOM_SLASH', sunbeam.action.ignore))

    :rtype: EclipseState
    """
    if isfile(deck):
        return EclipseState(lib.parse(deck, _parse_context(recovery)))
    return EclipseState(lib.parse_data(deck, _parse_context(recovery)))


def parse_deck(deck, keywords=[], recovery=[]):
    """Parse a deck from either a string or file.

    Args:
        deck (str): Either an eclipse deck string or path to a file to open.
        keywords (dict|[dict]): List of keyword parser extensions in opm-parser
            format. A description of the opm-parser keyword format can be found
            at: https://github.com/OPM/opm-parser/blob/master/docs/keywords.txt
        recovery ((str, action)|[(str, action)]): List of error recoveries.
            An error recovery is defined by a pair of a string naming the error
            event to be handled and the action taken for this error event. The
            named error event can be one of the following:
                "PARSE_UNKNOWN_KEYWORD"
                "PARSE_RANDOM_TEXT"
                "PARSE_RANDOM_SLASH"
                "PARSE_MISSING_DIMS_KEYWORD"
                "PARSE_EXTRA_DATA"
                "PARSE_MISSING_INCLUDE"
                "UNSUPPORTED_SCHEDULE_GEO_MODIFIER"
                "UNSUPPORTED_COMPORD_TYPE"
                "UNSUPPORTED_INITIAL_THPRES"
                "UNSUPPORTED_TERMINATE_IF_BHP"
                "INTERNAL_ERROR_UNINITIALIZED_THPRES"
                "SUMMARY_UNKNOWN_WELL"
                "SUMMARY_UNKNOWN_GROUP"
            The avaiable recovery actions can be one of the following:
                sunbeam.action.throw
                sunbeam.action.warn
                sunbeam.action.ignore

    Examples:
        Parses a deck from the string "RUNSPEC\\n\\nDIMENS\\n 2 2 1 /\\n"
            deck = sunbeam.parse_deck("RUNSPEC\\n\\nDIMENS\\n 2 2 1 /\\n")
        Parses a deck from the NORNE data set with recovery set to ignore
        PARSE_RANDOM_SLASH error events.
            deck = sunbeam.parse_deck('~/opm-data/norne/NORNE_ATW2013.DATA',
                recovery=('PARSE_RANDOM_SLASH', sunbeam.action.ignore))

    :rtype: sunbeam.libsunbeam.Deck
    """
    if keywords:
        # this might be a single keyword dictionary, in which case we pack it
        # into a list. If it's not a dict we assume it's an iterable and just
        # carry on
        if isinstance(keywords, dict):
            keywords = [keywords]
        keywords = list(map(json.dumps, keywords))
    is_file = isfile(deck) # If the deck is a file, the deck is read from
                           # that file. Otherwise it is assumed to be a
                           # string representation of the the deck.
    pc = _parse_context(recovery) if recovery else lib.ParseContext()
    return lib.parse_deck(deck, keywords, is_file, pc)
