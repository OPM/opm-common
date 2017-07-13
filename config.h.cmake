/* begin opm-parser
   put the definitions for config.h specific to
   your project here. Everything above will be
   overwritten
*/
/* begin private */
/* Name of package */
#define PACKAGE "@DUNE_MOD_NAME@"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "@DUNE_MAINTAINER@"

/* Define to the full name of this package. */
#define PACKAGE_NAME "@DUNE_MOD_NAME@"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "@DUNE_MOD_NAME@ @DUNE_MOD_VERSION@"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "@DUNE_MOD_NAME@"

/* Define to the home page for this package. */
#define PACKAGE_URL "@DUNE_MOD_URL@"

/* Define to the version of this package. */
#define PACKAGE_VERSION "@DUNE_MOD_VERSION@"

/* end private */

/* Define to the version of opm-parser */
#define OPM_PARSER_VERSION "${OPM_PARSER_VERSION}"

/* Define to the major version of opm-parser */
#define OPM_PARSER_VERSION_MAJOR ${OPM_PARSER_VERSION_MAJOR}

/* Define to the minor version of opm-parser */
#define OPM_PARSER_VERSION_MINOR ${OPM_PARSER_VERSION_MINOR}

/* Define to the revision of opm-parser */
#define OPM_PARSER_VERSION_REVISION ${OPM_PARSER_VERSION_REVISION}

/* Specify whether ERT/libecl is available or not */
#cmakedefine HAVE_ERT 1
#cmakedefine HAVE_LIBECL 1

/* begin bottom */

/* end bottom */

/* end opm-parser */
