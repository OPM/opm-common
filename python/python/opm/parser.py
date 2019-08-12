from __future__ import absolute_import
import os.path
import json

from opm import libopmcommon_python as lib
from .properties import SunbeamState


def _init_parse(recovery, keywords):
    context = lib.ParseContext(recovery)
    parser = lib.Parser()
    for kw in keywords:
        parser.add_keyword(json.dumps(kw))

    return (context,parser)


def parse(deck_file, recovery=[], keywords=[]):
    """Will parse a file and create a SunbeamState object.

    The parse function will parse a complete ECLIPSE input deck and return a
    SunbeamState instance which can be used to access all the properties of the
    Eclipse parser has internalized. Assuming the following small script has
    been executed:

        import sunbeam

        result = sunbeam.parse("ECLIPSE.DATA")

    Then the main results can be found in .deck, .state and .schedule
    properties of the result object:

    result.deck: This is the first result of the parsing process. In the Deck
    datastructure the original organisation with keywords found in the Eclipse
    datafile still remains, but the following processing has been completed:

      o All comments have been stripped out.

      o All include files have been loaded.

      o All values are converted to the correct type, i.e. string, integer or
        double, and floating point values have been converted to SI units.

      o '*' literals and values which have been omitted have been updated with
        the correct default values.

      o The content has been basically verified; at least datatypes and the
        number of records in keywords with a fixed number of records.

   You can always create a Deck data structure - even if your Eclipse input is
   far from complete, however this is quite coarse information - and if
   possible you are probably better off working with either the EclipseState
   object found in result.state or the Schedule object found in
   result.schedule.

   result.state: This is a more processed result, where the different keywords
   have been assembled into higher order objects, for instance the various
   keywords which together constitute one Eclipse simulationgrid have been
   assembled into a EclipseGrid class, the PERMX keywords - along with BOX
   modifiers and such have been assembled into a properties object and the
   various table objects have been assembled into Table class.


   result.schedule: All the static information is assembled in the state
   property, and all the dynamic information is in the schedule property. The
   schedule property is an instance of the Schedule class from opm-parser, and
   mainly consists of well and group related information, including all rate
   information.

   Example:

        import sunbeam
        result = sunbeam.parse("ECLIPSE.DATA")

        # Fetch the static properties from the result.state object:
        grid = result.state.grid
        print("The grid dimensions are: (%d,%d,%d)" % (grid.getNX(),
                                                       grid.getNY(),
                                                       grid.getNZ()))
        grid_properties = result.state.props()
        poro = grid_properties["PORO"]
        print("PORO[0]: %g" % poro[0])


        # Look at the dynamic properties:
        print("Wells: %s" % result.schedule.wells)


    The C++ implementation underlying opm-parser implemenest support for a
    large fraction of ECLIPSE properties, not all of that is exposed in Python,
    but it is quite simple to extend the Python wrapping.

    In addition to the deck_file argument the parse() function has two optional
    arguments which can be used to alter the parsing process:

    recovery: The specification of the ECLIPSE input format is not very strict,
      and out in the wild there are many decks which are handled corectly by
      ECLIPSE, although they seem to be in violation with the ECLIPSE input
      specification. Also there are *many* more exotic features of the ECLIPSE
      input specificiaction which are not yet handled by the opm-parser.

      By default the parser is quite strict, and when an unknown situation is
      encountered an exception will be raised - however for a set of recognized
      error conditions it is possible to configure the parser to ignore the
      errors. A quite common situation is for instance that an extra '/' is
      found dangling in the deck - this is probably safe to ignore:

          result = sunbeam.parse("ECLIPSE.DATE",
                                  recovery = [("PARSE_RANDOM_SLASH", sunbeam.action.ignore)])

      The full list of error modes which are recognized can be found in include
      file ParseContext.hpp in the opm-parser source. To disable errors using
      the recovery method is a slippery slope; you might very well end up
      masking real problems in your input deck - and the final error when
      things go *really atray* might be quite incomprihensible. If you can
      modify your input deck that is recommended before ignoring errors with
      the recovery mechanism.


    keywords: The total number of kewords supported by ECLIPSE is immense, and
      the parser code only supports a fraction of these. Using the keywords
      argumnt you can tell the parser about additional keywords. The keyword
      specifications should be supplied as Python dictionaries; see the
      share/keywords directories in the opm-parser source for syntax. Assuming
      you have an input deck with the keyword WECONCMF which opm-parser does
      not yet support. You could then add that to your parser in the following
      manner:

         import sunbeam

         weconmf = {"name" : "WECONMF",
                    "sections" : ["SCHEDULE"],
                    "items" : [{"name" : "well", "value_type" : "STRING"},
                               {"name" : "comp_index", "value_type" : "INTEGER"},
                               {"name" : "max_mole_fraction", "value_type" : "DOUBLE", "dimension" : "1"},
                               {"name" : "workover", "value_type : "STRING", "default" : "NONE"},
                               {"name" : "end_flag", "value_type": "STRING", "default" : "NO}]}

         state = sunbeam.parse("ECLIPSE.DATA", keywords = [weconmf])

      Adding keywords in this way will ensure that the relevant information is
      inernalized in the deck, but it will not be taken into account when
      constructing the EclipseState and Schedule objects.

    """

    if not os.path.isfile(deck_file):
        raise IOError("No such file: {}".format(deck_file))

    context, parser = _init_parse(recovery, keywords)
    return SunbeamState( lib.parse(deck_file, context, parser))



def parse_string(deck_string, recovery=[], keywords=[]):
    """Will parse a string and create SunbeamState object.

    See function parse() for further details about return type and the recovery
    and keyword arguments.

    """
    context, parser = _init_parse(recovery, keywords)
    return SunbeamState(lib.parse_string(deck_string, context, parser))



def load_deck(deck_file, keywords=[], recovery=[]):
    """
    Will parse a file and return a Deck object.

    See function parse() for details about the keywords and recovery arguments.
    """
    if not os.path.isfile(deck_file):
        raise IOError("No suc file: {}".format(deck_file))

    context, parser = _init_parse(recovery, keywords)
    return lib.create_deck(deck_file, context, parser)


def load_deck_string(deck_string, recovery=[], keywords=[]):
    """
    Will parse a string and return a Deck object.

    See function parse() for details about the keywords and recovery arguments.
    """
    context, parser = _init_parse(recovery, keywords)
    return lib.create_deck_string(deck_string, context, parser)

