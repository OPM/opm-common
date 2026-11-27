# opm-common jenkins build scripts:

**build-opm-module.sh**:
This is a helper script which contains functions for building,
testing and cloning modules.

**build.sh**:
This expects to run on a jenkins instance with opm-common as the 'origin' remote.

It will build and test opm-common. It can be used both for post-merge builds
of the master branch and for a github pull request builder job.

**static_analysis.sh**:
This expects to run on a jenkins instance with opm-common as the 'origin' remote.

It will run and build opm-common using various configurations, and then
run static analysis.

# Jenkins pipeline

The `jenkins/` directory is a Jenkins shared library, `opm`, used by the
`jenkins/Jenkinsfile` of each module:

    @Library('opm') _

    opmModule()

`opmModule()` runs the module's `jenkins/build.sh` in the OPM CI docker
image (`OPM_CI_DOCKER`), as parallel stages in a single pipeline, one per
build configuration selected by `build-configurations.sh` in the image. Each
configuration reports its own commit status (`jenkins/<configuration>`) and
test results, and wipes its workspace afterwards. The gcc or clang, cmake
and nvcc warnings in the build output are recorded as
`Warnings (<configuration>)`. When produced, the reference data differences
(`data_diff`), the failure report plots and the Norne well plots are archived.

The trigger comment works as before: downstreams are built unless it says
`no downstreams`. `update_data`, `wheels` and `run` (`norne`,
`norne_parallel`) need the build of the `default` configuration, and are only
passed to its stage. The build fails early if `default` is not built.
The optional arguments of `opmModule()` are described in
`jenkins/vars/opmModule.groovy`.

## Jenkins setup

Required plugins: Pipeline, GitHub Branch Source, GitHub PR Comment Build,
Pipeline: GitHub Notify Step, JUnit, Warnings Next Generation and Email
Extension. The builder needs docker, and the
`OPM_CI_DOCKER` environment variable naming the image.

1. Manage Jenkins → System → Global Trusted Pipeline Libraries: add a library
   named `opm`, default version `master`, retrieved with Git from
   https://github.com/OPM/opm-common with Library Path `jenkins/`.
2. Create a multibranch pipeline per module repository using the GitHub
   branch source, with Script Path `jenkins/Jenkinsfile`.
3. Under the branch source, use "Named branches get different properties":
   by default "Suppress automatic SCM triggering" and "Trigger build on pull
   request comment" with the expression `jenkins build this.*please`, and an
   exception for `master` without properties.
4. Make sure the repository webhooks deliver push, pull request and issue
   comment events to Jenkins.
5. Manage Jenkins → System → Extended E-mail Notification: configure the
   SMTP server, and set Default Recipients to those who should be told about
   every failed master build. The authors of the changes since the last
   successful build are emailed as well.
6. Manage Jenkins → System → Groovy Based Warnings Parsers: add a parser with
   ID `nvcc` for the warnings of nvcc's front end, which no built-in parser
   recognises. Host compiler warnings are found by the gcc and clang parsers.
   Regular expression:

       ^(.+?)\((\d+)\): (warning|error)(?: #(\d+)(?:-D)?)?: (.*)$

   Mapping script:

       import edu.hm.hafner.analysis.Severity

       builder.setFileName(matcher.group(1))
              .setLineStart(Integer.parseInt(matcher.group(2)))
              .setSeverity(matcher.group(3) == 'error' ? Severity.ERROR : Severity.WARNING_NORMAL)
              .setCategory(matcher.group(4) ?: '')
              .setMessage(matcher.group(5))
              .setType('nvcc')
       return builder.buildOptional()

   Example: `src/kernel.cu(42): warning #177-D: variable "x" was declared but never referenced`
