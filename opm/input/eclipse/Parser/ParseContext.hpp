/*
  Copyright 2015 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPM_PARSE_CONTEXT_HPP
#define OPM_PARSE_CONTEXT_HPP

#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Opm {

enum class InputErrorAction;

class ErrorGuard;
class KeywordLocation;

} // namespace Opm

namespace Opm {

    /// Control parser behaviour in failure conditions.
    ///
    /// The ParseContext class is a way to influence the parser's behaviour
    /// and the EclipseState/Schedule construction phase in the face of
    /// errors or inconsistencies in the input stream.
    ///
    /// For each of the possible problem categories encountered, the
    /// possible actions are goverened by the InputErrorAction enum:
    ///
    ///     InputErrorAction::THROW_EXCEPTION
    ///        -> Throws an exception at the point of call which will
    ///           typically lead to the application shutting down shortly
    ///           thereafter.  Might also include a back-trace for
    ///           subsequent analysis.
    ///
    ///     InputErrorAction::WARN
    ///       -> Warns about a potential problem, but continues loading and
    ///          analysing the input deck.
    ///
    ///     InputErrorAction::IGNORE
    ///       -> Ignores the problem without issuing any diagnostic message.
    ///
    ///     InputErrorAction::EXIT1
    ///       -> Stops parsing and shuts down the application immediately,
    ///          with a status code signifying an error condition.  This
    ///          action should only be used for the most severe cases, in
    ///          which the input analysis cannot proceed.  One example of
    ///          this situation would be a missing INCLUDE file.
    ///
    ///     InputErrorAction::DELAYED_EXIT1
    ///       -> Schedule application shutdown, with a failure condition
    ///          status code, at the end of loading the input file.  Should
    ///          typically be used only for cases that cannot be simulated,
    ///          but for which the parser is able to continue and possibly
    ///          diagnose other inconsistencies.
    ///
    /// The internal datastructure is a map from categories (strings) to
    /// action values (of type InputErrorAction).  The context categories
    /// are intended to be descriptive and human readable, like
    ///
    ///     "PARSE_RANDOMTEXT"
    ///
    /// The constructors will furthermore inspect the environment variables
    /// OPM_ERRORS_IGNORE, OPM_ERRORS_WARN, OPM_ERRORS_EXIT1,
    /// OPM_ERRORS_DELAYED_EXIT1 and OPM_ERRORS_EXCEPTION when forming the
    /// initial set of context actions.  These variables should be set as
    /// strings of update syntax, and the categories referenced by these
    /// variables will have their actions reset to that implied by the
    /// variable name.  As an example, categories referenced by the
    /// OPM_ERRORS_DELAYED_EXIT1 enviroment variable will have their actions
    /// reset to InputErrorAction::DELAYED_EXIT1.
    ///
    /// Update syntax: The main function for updating the policy of a
    /// parseContext instance is the update() method.  This member function
    /// takes a string as input, and resets the actions for categories that
    /// match the string.  In particular, the string can contain shell-style
    /// wildcards ('*' and '?' as though matched by the Posix function
    /// fnmatch()), and is split on ':' or '|' to allow multiple settings to
    /// be applied in one go:
    ///
    /// Reset one context category:
    ///     update("PARSE_RANDOM_SLASH", InputErrorAction::IGNORE)
    ///
    /// Ignore all unsupported features:
    ///     update("UNSUPPORTED_*", InputErrorAction::IGNORE)
    ///
    /// Reset two categories (names separated by ':'):
    ///     update("UNSUPPORTED_INITIAL_THPRES:PARSE_RANDOM_SLASH",
    ///            InputErrorAction::IGNORE)
    ///
    /// The update function will silently ignore unknown context categories.
    /// On the other hand, member function updateKey() will throw an
    /// exception for any unknown category.
    class ParseContext
    {
    public:
        /// Default constructor.
        ///
        /// Creates a context object with all known categories intialised to
        /// their default action.  Some/all categories may be overridden
        /// through environment variables.
        ParseContext();

        /// Constructor.
        ///
        /// Context object with all known categories initialised to a single
        /// user-defined action.  Some/all categories may be overridden
        /// through environment variables.
        ///
        /// \param[in] default_action Action for all context categories that
        /// are not overridden by environment variables.
        explicit ParseContext(InputErrorAction default_action);

        /// Constructor.
        ///
        /// Context object with all known categories initialised to their
        /// default action, except for those categories that are explicitly
        /// assigned a user-defined action through the constructor argument.
        /// Some/all categories may be overridden through environment
        /// variables.
        ///
        /// \param[in] initial User-defined action for select context
        /// categories.
        explicit ParseContext(const std::vector<std::pair<std::string, InputErrorAction>>& initial);

        /// Handle an input error.
        ///
        /// This is the primary client interface that starts input failure
        /// processing.  Each failure category will be handled according to
        /// its currently configured action.
        ///
        /// \param[in] errorKey Failure category.
        ///
        /// \param[in] msg Diagnostic message.  Will be transformed through
        /// \code OpmInputError::format() \endcode if call site additionally
        /// passes a \c KeywordLocation for the error handling.
        ///
        /// \param[in] location Input file position of the keyword that
        /// triggered this failure condition.  Nullopt if no such location
        /// is available.
        ///
        /// \param[in,out] errors All diagnostic messages collected thus
        /// far.  If the configured action for \p errorKey is WARN or
        /// DELAYED_EXIT1, the \p errors object will include the diagnostic
        /// message derived from \p msg as either a warning or an error.
        void handleError(const std::string&                    errorKey,
                         const std::string&                    msg,
                         const std::optional<KeywordLocation>& location,
                         ErrorGuard&                           errors)  const;

        /// Handle an unknown keyword in the input stream.
        ///
        /// This function exists mostly for backwards compatibility.
        ///
        /// \param[in] keyword Unknown keyword in input stream.
        ///
        /// \param[in] location Input file position of the unknown \p
        /// keyword.  Nullopt if no such location is available.
        ///
        /// \param[in,out] errors All diagnostic messages collected thus
        /// far.  If the \p keyword is not explicitly ignored--see member
        /// function ignoreKeyword()--the \p errors object will include a
        /// diagnostic message for this unknown keyword.
        void handleUnknownKeyword(const std::string&                    keyword,
                                  const std::optional<KeywordLocation>& location,
                                  ErrorGuard&                           errors) const;

        /// Existence predicate for particular context category.
        ///
        /// \param[in] key Context category.
        ///
        /// \return Whether or not the category \p key is known to this
        /// context object.
        bool hasKey(const std::string& key) const;

        /// Reset action for particular context category.
        ///
        /// Throws an exception of type \code std::invalid_argument \endcode
        /// if the context category is unknown.
        ///
        /// \param[in] key Context category.
        ///
        /// \param[in] action New action for category \p key.
        void updateKey(const std::string& key, InputErrorAction action);

        /// Reset action for all context categories.
        ///
        /// \param[in] action New action for all known context categories.
        void update(InputErrorAction action);

        /// Reset action for one or more context categories.
        ///
        /// This is the most general update function.  The input key string
        /// is treated as a "category selection string", and all context
        /// categories matching a pattern will reset their action.  The
        /// algorithm for decoding the category selection string is:
        ///
        ///   1. Split category selection string into elements on occurences
        ///      of ':' or '|', and then each element is treated separately.
        ///
        ///   2. For each element in the list from 1):
        ///
        ///      a) If the element contains a wildcard ('*'), then treat the
        ///         element as a shell-style pattern and update all context
        ///         categories matching this pattern.
        ///
        ///      b) Otherwise, if the element is exactly equal to a known
        ///         context category, then update that category.
        ///
        ///      c) Otherwise, silently ignore the element.
        ///
        /// \param[in] keyString Category selection string.
        ///
        /// \param[in] action New action for those categories matching \p
        /// keyString.
        void update(const std::string& keyString, InputErrorAction action);

        /// Ignore particular unknown input keyword if encountered during
        /// parsing.
        ///
        /// Bypasses the error handling in handleUnknownKeyword().
        ///
        /// [2026-01-19] Useful in the early development of the OPM input
        /// parser, but exists now mostly for backwards compatibility and
        /// historical reasons.  See Issue OPM/opm-simulators#6541 for why
        /// we might want to remove this function.
        ///
        /// \param[in] keyword Unknown input keyword that should be
        /// explicitly ignored if encountered in the input stream.
        void ignoreKeyword(const std::string& keyword);

        /// Retrieve category action for particular context category.
        ///
        /// \param[in] key Context category.
        ///
        /// \return Category action for \p key.
        InputErrorAction get(const std::string& key) const;

        /// Define action for user-specified category.
        ///
        /// \param[in] key User-specified context category.  If \p key
        /// already exists in this context object, then the context object
        /// is unchanged.  Use member functions update() or updateKey() to
        /// change the action of an existing context category.
        ///
        /// \param[in] default_action Default action for user-specified
        /// context category \p key.
        void addKey(const std::string& key, InputErrorAction default_action);

        /// Define how to handle simulator specific keyword suppression.
        ///
        /// In particular, this function defines how the parser will treat
        /// the SKIP100 and SKIP300 keywords.  Keywords between SKIP/ENDSKIP
        /// are always suppressed/ignored/skipped.
        ///
        /// \param[in] skip_mode Mode of operation for the keyword
        /// suppression algorithm.  Supported values are:
        ///
        ///   -  "100" -- Skip/ignore keywords between SKIP100/ENDSKIP.  Keep
        ///               others.  Default setting.
        ///
        ///   -  "300" -- Skip/ignore keywords between SKIP300/ENDSKIP.  Keep
        ///               others.
        ///
        ///   -  "all" -- Skip/ignore keywords both between SKIP100/ENDSKIP and
        ///               between SKIP300/ENDSKIP.
        void setInputSkipMode(const std::string& skip_mode);

        /// Whether or not a particular keyword activates keyword
        /// suppression.
        ///
        /// \param[in] deck_name Keyword name.
        ///
        /// \return Whether or not \p deck_name is one of the keywords that
        /// activates keyword suppression.  Results depend on the input skip
        /// mode defined through setInputSkipMode().
        bool isActiveSkipKeyword(const std::string& deck_name) const;

        /// The PARSE_EXTRA_RECORDS field controls the parser's response to
        /// keywords whose size has been defined in an earlier keyword.
        ///
        /// Example:
        ///
        ///   EQLDIMS
        ///     2  100  20  1  1  /
        ///   -- ...
        ///   EQUIL
        ///    2469   382.4   1705.0  0.0    500    0.0 1 1  20 /
        ///    2469   382.4   1705.0  0.0    500    0.0 1 1  20 /
        ///    2470   382.4   1705.0  0.0    500    0.0 1 1  20 /
        ///
        /// Item 1 of EQLDIMS is 2 which determines the number of expected
        /// records in EQUIL.  Since there are 3 records in this EQUIL
        /// keyword however, this generates an error condition that must be
        /// handled by the parser.
        const static std::string PARSE_EXTRA_RECORDS;

        /// The unknown keyword category controls the parser's behaviour on
        /// encountering an unknown keyword.  Observe that 'keyword' in this
        /// context means
        ///
        ///    a string of at most eight upper case letters and numbers,
        ///    starting with an upper case letter.
        ///
        /// Moreover, the unknown keyword handling does not inspect any
        /// collection of keywords to determine if a particular string
        /// corresponds to a known, valid keyword which just happens to be
        /// ignored for this particular parse operation.
        ///
        /// Finally, the "unknown keyword" and "random text" categories are
        /// not fully independent.  As a result, encountering an unknown
        /// keyword without halting the parser might lead to a subsequent
        /// piece of "random text" not being correctly identified as such.
        const static std::string PARSE_UNKNOWN_KEYWORD;

        /// Random text is an input deck string not correctly formatted as a
        /// keyword heading.
        const static std::string PARSE_RANDOM_TEXT;

        /// It turns out that random '/'--i.e. typically an extra slash
        /// which is not needed--is quite common.  This is therefore a
        /// special case treatment of the "random text" behaviour.
        const static std::string PARSE_RANDOM_SLASH;

        /// For some keywords the number of records (i.e., size) is given as
        /// an item in another keyword.  A typical example is the EQUIL
        /// keyword where the number of records is given by the NTEQUL item
        /// of the EQLDIMS keyword.  If the size defining XXXDIMS keyword is
        /// not in the deck, we can use the default values of the XXXDIMS
        /// keyword.  This is controlled by the "missing dimension keyword"
        /// field.
        ///
        /// Observe that a fully defaulted XXXDIMS keyword does not trigger
        /// this behaviour.
        const static std::string PARSE_MISSING_DIMS_KEYWORD;

        /// If the number of elements in the input record exceeds the number
        /// of items in the keyword configuration this error situation will
        /// be triggered.  Many keywords end with several ECLIPSE300 only
        /// items--in some cases we have omitted those items in the Json
        /// configuration; that will typically trigger this error situation
        /// when encountering an ECLIPSE300 deck.
        const static std::string PARSE_EXTRA_DATA;

        /// If an include file is not found we can configure the parser to
        /// continue reading.  The resulting deck will probably not be
        /// consistent in this case.
        const static std::string PARSE_MISSING_INCLUDE;

        /// Certain keywords require, or prohibit, other specific keywords.
        /// When such keywords are found in an invalid combination (i.e.,
        /// required keyword missing required or prohibited keyword
        /// present), this error situation occurs.
        const static std::string PARSE_INVALID_KEYWORD_COMBINATION;

        /// Dynamic number of wells exceeds maximum declared in
        /// RUNSPEC keyword WELLDIMS (item 1).
        const static std::string RUNSPEC_NUMWELLS_TOO_LARGE;

        /// Dynamic number of connections per well exceeds maximum
        /// declared in RUNSPEC keyword WELLDIMS (item 2).
        const static std::string RUNSPEC_CONNS_PER_WELL_TOO_LARGE;

        /// Dynamic number of groups exceeds maximum number declared in
        /// RUNSPEC keyword WELLDIMS (item 3).
        const static std::string RUNSPEC_NUMGROUPS_TOO_LARGE;

        /// Dynamic group size exceeds maximum number declared in
        /// RUNSPEC keyword WELLDIMS (item 4).
        const static std::string RUNSPEC_GROUPSIZE_TOO_LARGE;

        /// Dynamic number of multi-segmented wells exceeds maximum declared
        /// in RUNSPEC keyword WSEGDIMS (item 1).
        const static std::string RUNSPEC_NUMMSW_TOO_LARGE;

        /// Dynamic number of segments per MS well exceeds maximum declared
        /// in RUNSPEC keyword WSEGDIMS (item 2).
        const static std::string RUNSPEC_NUMSEG_PER_WELL_TOO_LARGE;

        /// Dynamic number of branches exceeds maximum number declared in
        /// RUNSPEC keyword WSEGDIMS (item 3).
        const static std::string RUNSPEC_NUMBRANCH_TOO_LARGE;

        /// Should we allow keywords of length more than eight characters?
        /// If the keyword is too long it will be internalized using only
        /// the eight first characters.
        const static std::string PARSE_LONG_KEYWORD;

        /// The unit system specified via the FILEUNIT keyword is different
        /// from the unit system used by the deck.
        const static std::string UNIT_SYSTEM_MISMATCH;

        /// If the third item in the THPRES keyword is defaulted, the
        /// threshold pressure is inferred from the initial pressure.
        /// This is currently not supported.
        const static std::string UNSUPPORTED_INITIAL_THPRES;

        /// If the second item in the WHISTCTL keyword is set to YES.
        ///
        /// The simulator is supposed to terminate if the well is changed to
        /// BHP control.  This feature is not yet supported.
        const static std::string UNSUPPORTED_TERMINATE_IF_BHP;

        /// Parser fails to analyse the defining expression of a UDQ.
        const static std::string UDQ_PARSE_ERROR;

        /// Parser unable to establish a coherent UDQ set type for a
        /// user-defined quantity.
        const static std::string UDQ_TYPE_ERROR;

        /// Cannot evaluate the defining expression of a UDQ at the point of
        /// definition due to missing objects, e.g., wells or groups.
        const static std::string UDQ_DEFINE_CANNOT_EVAL;

        /// If the third item in the THPRES keyword is defaulted the
        /// threshold pressure is inferred from the initial pressure--if you
        /// still ask the ThresholdPressure instance for a pressure value
        /// this error will be signalled.  This is currently not supported.
        const static std::string INTERNAL_ERROR_UNINITIALIZED_THPRES;

        /// If the deck does not have all sections, whence complete
        /// EclipseState and Schedule objects cannot be constructed, we may
        /// still be able to construct a slim EclipseGrid.
        const static std::string PARSE_MISSING_SECTIONS;

        /// When defining wells and groups with the WELSPECS and GRUPTREE
        /// keywords we do not allow leading or trailing spaces.  The code
        /// in Schedule.cpp will *unconditionally* remove the spaces, but
        /// with PARSE_WGNAME_SPACE setting you can additionally configure
        /// the normal IGNORE|WARN|ERROR behavior.
        const static std::string PARSE_WGNAME_SPACE;

        /// Well level summary vector references an unknown well.
        const static std::string SUMMARY_UNKNOWN_WELL;

        /// Group level summary vector references an unknown group.
        const static std::string SUMMARY_UNKNOWN_GROUP;

        /// Summary vector references an unknown network node.
        const static std::string SUMMARY_UNKNOWN_NODE;

        /// Aquifer level summary vector references an unknown aquifer
        /// (analytic or numeric).
        const static std::string SUMMARY_UNKNOWN_AQUIFER;

        /// Summary vector name is unknown.
        const static std::string SUMMARY_UNHANDLED_KEYWORD;

        /// Summary vector references an undefined UDQ.
        const static std::string SUMMARY_UNDEFINED_UDQ;

        /// User-defined quantity does not have an associated unit of
        /// measure and will thus be reported without any units.
        const static std::string SUMMARY_UDQ_MISSING_UNIT;

        /// Summary vector references an unknown FIP region.
        const static std::string SUMMARY_INVALID_FIPNUM;

        /// Summary vector references an empty region.
        const static std::string SUMMARY_EMPTY_REGION;

        /// Summary vector references an out-of-bounds region ID.
        const static std::string SUMMARY_REGION_TOO_LARGE;

        /// A well or group name used before it has been fully defined
        /// through WELSPECS/COMPDAT/GRUPTREE.
        const static std::string SCHEDULE_INVALID_NAME;

        // Only explicitly supported keywords can be included in an ACTIONX
        // or PYACTION block.  These categories control what should happen
        // when encountering an illegal keyword in such blocks.

        /// ACTIONX block uses an unsupported schedule keyword.
        const static std::string ACTIONX_ILLEGAL_KEYWORD;

        /// PYACTION block uses an unsupported schedule keyword.
        const static std::string PYACTION_ILLEGAL_KEYWORD;

        ///  Error flag marking parser errors ic ACTIONX conditions
        const static std::string ACTIONX_CONDITION_ERROR;

        /// Error flag marking that an ACTIONX has no condition
        const static std::string ACTIONX_NO_CONDITION;

        /// The RPTRST, RPTSOL and RPTSCHED keywords have two alternative
        /// forms.  The traditional style uses integer controls, whence the
        /// RPTRST keyword can be configured along the lines of:
        ///
        ///   RPTRST
        ///      0 0 0 1 0 1 0 2 0 0 0 0 0 1 0 0 2 /
        ///
        /// The recommended way uses string mnemonics which can optionally
        /// have an integer value, e.g., something along the lines of
        ///
        ///   RPTRST
        ///      BASIC=2  FLOWS  ALLPROPS /
        ///
        /// It is not possible to mix the two styles within a single keyword
        /// instance, though a particlar run may use both styles within a
        /// single model description as long as the instances are separate.
        ///
        /// A situation with mixed input style is identified if any of the
        /// items are exclusively integers while others are string
        /// mnemonics.  To avoid the situation in which values in the
        /// assignments like BASIC=2 be interpreted as integers, there
        /// should be no space character on either side of the '='
        /// character.  We nevertheless support slightly relaxed parsing for
        /// situations like
        ///
        ///    RPTRST
        ///       BASIC = 2 /
        ///
        /// where the intention is clear.  The RPT_MIXED_STYLE category
        /// tries to handle this situation.  Observe that really mixed input
        /// style is impossible to handle and will lead to a hard exception.
        /// RPT_MIXED_STYLE nevertheless enables configuring lenient
        /// behaviour in interpreting the input as string mnemonics.
        const static std::string RPT_MIXED_STYLE;

        /// An unknown mnemonic in one of the RPT* keywords.
        const static std::string RPT_UNKNOWN_MNEMONIC;

        /// Operation applied to incorrect/unknown group.
        const static std::string SCHEDULE_GROUP_ERROR;

        /// Explicitly supplied guide rate will be ignored.
        const static std::string SCHEDULE_IGNORED_GUIDE_RATE;

        /// Well parented directly to the FIELD group.
        ///
        /// Typically generates a warning.
        const static std::string SCHEDULE_WELL_IN_FIELD_GROUP;

        /// COMPSEGS data invalid in some way.
        ///
        /// For instance, referencing non-existent segments or not covering
        /// all connections of a single well.
        const static std::string SCHEDULE_COMPSEGS_INVALID;

        /// COMPSEGS definition not supported.
        ///
        /// Might for instance use an unsupported MD specification.
        const static std::string SCHEDULE_COMPSEGS_NOT_SUPPORTED;

        /// Connection data (COMPDAT keyword) invalid in some way.
        const static std::string SCHEDULE_COMPDAT_INVALID;

        /// ICD keyword (WSEGAICD, WSEGSICD, WSEGVALV) references a missing
        /// well segment.
        ///
        /// Typically generates a warning and drops the device.  Note,
        /// however, that there are likely to be other issues in the input
        /// deck when this situation occurs.
        const static std::string SCHEDULE_ICD_MISSING_SEGMENT;

        /// ICD keyword (WSEGAICD, WSEGSICD, WSEGVALV) is not compatible
        /// with the pressure drop model chosen for a particular MSW.
        const static std::string SCHEDULE_ICD_INCOMPATIBLE_PDROP_MODEL;

        // The SIMULATOR_KEYWORD_ categories are intended to define the
        // parser behaviour for when the parser itself recognises an input
        // keyword, but the simulator does not support the intended use of
        // that keyword.

        /// Keyword that is not supported in the simulator.
        const static std::string SIMULATOR_KEYWORD_NOT_SUPPORTED;

        /// Keyword that is not supported in the simulator, and which should
        /// be treated as a critical failure if encountered.
        const static std::string SIMULATOR_KEYWORD_NOT_SUPPORTED_CRITICAL;

        /// Keyword item setting that is not supported in the simulator.
        const static std::string SIMULATOR_KEYWORD_ITEM_NOT_SUPPORTED;

        /// Keyword item setting that is not supported in the simulator and
        /// which should be treated as a critical failure if encountered.
        const static std::string SIMULATOR_KEYWORD_ITEM_NOT_SUPPORTED_CRITICAL;

    private:
        /// Current action for all known context categories.
        std::map<std::string, InputErrorAction> m_errorContexts{};

        /// Keywords unknown to the parser, and that should just be ignored
        /// in the input stream.
        std::set<std::string> ignore_keywords{};

        /// Compatibility mode for SKIP100/SKIP300 keywords.
        ///
        /// Supported values are:
        ///
        ///   -  "100" -- Skip keywords between SKIP100/ENDSKIP.  Keep
        ///               others.  Default setting.
        ///
        ///   -  "300" -- Skip keywords between SKIP300/ENDSKIP.  Keep
        ///               others.
        ///
        ///   -  "all" -- Skip keywords between SKIP100/ENDSKIP and
        ///               SKIP300/ENDSKIP.
        std::string m_input_skip_mode{"100"};

        /// Assign default actions for all known context categories.
        void initDefault();

        /// Override action for categories defined in environment.
        ///
        ///    - OPM_ERRORS_EXCEPTION: All categories here get THROW_EXCEPTION.
        ///    - OPM_ERRORS_WARN: All categories here get WARN.
        ///    - OPM_ERRORS_IGNORE: All categories here get IGNORE.
        ///    - OPM_ERRORS_EXIT1: All categories here get EXIT1.
        ///    - OPM_ERRORS_EXIT: All categories here get EXIT1.
        ///    - OPM_ERRORS_DELAYED_EXIT1: All categories here get DELAYED_EXIT1.
        ///    - OPM_ERRORS_DELAYED_EXIT: All categories here get DELAYED_EXIT1.
        void initEnv();

        /// Override action for all categories in a single environment variable
        ///
        /// \param[in] envVariable Name of environment variable.  Expected
        /// to be one of the OPM_ERRORS_* variables mentioned in initEnv().
        ///
        /// \param[in] action Context action for all categories referenced
        /// by \p envVariable.
        void envUpdate(const std::string& envVariable, InputErrorAction action);

        /// Override action for all categories matching a shell pattern.
        ///
        /// \param[in] pattern Shell-style string pattern against which the
        /// known context categories will be matched.
        ///
        /// \param[in] action Context action for all categories matching \p
        /// pattern.
        void patternUpdate(const std::string& pattern, InputErrorAction action);
    };

} // namespace Opm

#endif // OPM_PARSE_CONTEXT_HPP
