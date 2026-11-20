// Builds an OPM module in a multibranch pipeline, one parallel stage per
// build configuration. Each stage runs the module's jenkins/build.sh in the
// OPM CI docker image, with the environment the old pull request builder
// jobs provided.
//
// The build configurations are selected from the trigger comment by
// build-configurations.sh in the image. Trigger keywords which need the
// build of a particular configuration are only passed to the stage
// building it, and fail the build if that configuration is not built.
// update_data, run (norne and norne_parallel) and wheels all need default.
//
// Arguments (all optional):
//   module         - name of the module (default: the repository name)
//   image          - docker image to build in (default: env.OPM_CI_DOCKER)
//   dockerArgs     - additional docker run arguments, e.g. volumes
//   hostScripts    - directory with the wheels scripts on the builder
//                    (default '/var/lib/jenkins/build-scripts')
//   downstreams    - build downstreams unless the comment says
//                    'no downstreams' (default true)
//   label          - agent label to run the configurations on
//   statusContexts - report a commit status per configuration
//                    (default true, requires pipeline-githubnotify-step)
//   branchTriggers - map of branch name to the trigger comment to use for
//                    builds of that branch
//   keepWorkspace  - keep the workspaces after the build, e.g. for debugging
//                    (default false)
//   nvccParser     - id of a Groovy based warnings parser for nvcc,
//                    defined in the Jenkins configuration
//   notifyBranches - branches whose failed or unstable builds are emailed to
//                    the authors of the changes and Jenkins' default
//                    recipients (default ['master'], requires email-ext)
def call(Map args = [:]) {
  Map opts = [module: args.module ?: repositoryName(),
              image: args.image ?: env.OPM_CI_DOCKER,
              dockerArgs: args.dockerArgs ?: '--shm-size=2048m --cap-add=SYS_PTRACE ' +
                                              '-v /var/lib/jenkins/.ccache-docker:/ccache ' +
                                              '-v /var/lib/jenkins/opm-tests-updater/workspace/opm-tests:/opm-tests ' +
                                              '-v /var/lib/jenkins/.ssh:/home/ubuntu/.ssh:ro',
              hostScripts: args.hostScripts ?: '/var/lib/jenkins/build-scripts',
              statusContexts: args.containsKey('statusContexts') ? args.statusContexts : true,
              keepWorkspace: args.keepWorkspace ?: false,
              nvccParser: args.nvccParser]
  String label = args.label
  if (!opts.image)
    error 'No docker image given, set OPM_CI_DOCKER or pass image'

  try {
    buildAll(args, opts, label)
  } catch (err) {
    currentBuild.result = err instanceof org.jenkinsci.plugins.workflow.steps.FlowInterruptedException ?
                          'ABORTED' : 'FAILURE'
    throw err
  } finally {
    notifyFailure(args.notifyBranches ?: ['master'])
  }
}


def buildAll(Map args, Map opts, String label) {
  String comment = opmTriggerComment()
  if (comment)
    echo "Triggered by comment: ${comment}"
  else if (!env.CHANGE_ID && args.branchTriggers?.get(env.BRANCH_NAME)) {
    comment = args.branchTriggers[env.BRANCH_NAME]
    echo "Using trigger for branch ${env.BRANCH_NAME}: ${comment}"
  }
  boolean downstreams = args.containsKey('downstreams') ? args.downstreams : true
  comment = addDownstreams(comment, downstreams)

  // The configurations depend on the trigger comment
  List names
  node(label) {
    withEnv(["ghprbCommentBody=${comment}", "OPM_IMAGE=${opts.image}"]) {
      names = sh(returnStdout: true,
                 script: 'docker run --rm -e ghprbCommentBody --entrypoint /bin/bash "$OPM_IMAGE" ' +
                         '-c \'source $TOOLCHAIN_DIR/build-configurations.sh; echo $BTYPES\'').trim().tokenize()
    }
  }
  if (!names)
    error 'No build configurations selected'

  Map route = routeTriggers(comment, names)
  if (route.error)
    error route.error
  opts.updateData = route.updateData
  opts.wheels = route.wheels
  if (route.updateData || route.wheels || route.run)
    echo "Triggers for the default configuration: update_data=${route.updateData}, " +
         "wheels=${route.wheels}, run=${route.run}"

  def branches = [:]
  for (int i = 0; i < names.size(); ++i) {
    String name = names[i]
    String stageComment = commentForConfig(comment, name)
    branches[name] = {
      node(label) {
        stage(name) {
          buildConfig(name, stageComment, opts)
        }
      }
    }
  }

  parallel branches
}


def buildConfig(String name, String comment, Map opts) {
  String context = "jenkins/${name}"
  if (opts.statusContexts)
    githubNotify context: context, status: 'PENDING', description: 'Building'

  boolean failed = false
  try {
    deleteDir()
    checkout scm
    String sha1 = env.CHANGE_ID ? "pull/${env.CHANGE_ID}/merge" : env.BRANCH_NAME

    // Select the toolchain and build type belonging to this configuration.
    // This runs in the container, replacing its entry point. The name is
    // not passed as BTYPE, as build-configurations.sh uses that variable.
    writeFile file: '.jenkins-build-config.sh', text: '''#!/bin/bash
source $TOOLCHAIN_DIR/build-configurations.sh
types=($BTYPES)
toolchains=($CMAKE_TOOLCHAIN_FILES)
buildconfigs=($BUILD_CONFIGS)
for i in ${!types[@]}
do
  if [ "${types[$i]}" = "$OPM_CONFIGURATION" ]
  then
    export BTYPES=$OPM_CONFIGURATION
    export CMAKE_TOOLCHAIN_FILES=${toolchains[$i]}
    export BUILD_CONFIGS=${buildconfigs[$i]}
    exec /build/jenkins/build.sh
  fi
done
echo "Configuration $OPM_CONFIGURATION not found in $TOOLCHAIN_DIR/build-configurations.sh"
exit 1
'''

    withEnv(["sha1=${sha1}",
             "ghprbCommentBody=${comment}",
             "OPM_CONFIGURATION=${name}",
             "OPM_IMAGE=${opts.image}"]) {
      // The output is also kept in a log of its own for the warnings parsers,
      // as the console log mixes the parallel configurations
      sh "#!/bin/bash\nset -o pipefail\n" +
         "docker run ${opts.dockerArgs} --rm -u \$(id -u) -v \"\$WORKSPACE\":/build " +
         '-e sha1 -e ghprbCommentBody -e OPM_CONFIGURATION ' +
         '--entrypoint /bin/bash "$OPM_IMAGE" /build/.jenkins-build-config.sh ' +
         "2>&1 | tee build-${name}.log"

      // Steps on the builder which need the default configuration
      if (name == 'default' && opts.updateData)
        sh "rm -f deps/${opts.module} && ln -sf \"\$WORKSPACE\" deps/${opts.module} && " +
           "deps/opm-common/jenkins/open-data-update-pr.sh ${opts.module}"
      if (name == 'default' && opts.wheels) {
        sh "${opts.hostScripts}/build-pypi-pr.sh ${opts.module}"
        sh "${opts.hostScripts}/test-pypi-wheels.sh \"\$WORKSPACE/deps/opm-simulators\" \"\$WORKSPACE\""
        sh 'docker system prune -f'
      }
    }
  } catch (err) {
    failed = true
    if (opts.statusContexts)
      githubNotify context: context, status: 'FAILURE', description: 'Build failed'
    throw err
  } finally {
    // Ensure the test report is collected even if a downstream fails to build.
    // Each tested module has its own report, the parallel stages keep the
    // configurations apart.
    def results = junit testResults: "${name}/build-*/testoutput.xml", allowEmptyResults: true
    if (opts.statusContexts && !failed) {
      if (results.failCount > 0)
        githubNotify context: context, status: 'FAILURE',
                     description: "${results.failCount} of ${results.totalCount} tests failed"
      else
        githubNotify context: context, status: 'SUCCESS',
                     description: "${results.totalCount} tests passed"
    }

    recordWarnings(name, opts.nvccParser)

    // Produced only when tests fail or by the update_data and run triggers
    archiveArtifacts artifacts: "data_diff, ${name}/build-opm-simulators/failure_report/*.pdf, " +
                                'deps/opm-tests/norne/norne-wells*.pdf',
                     allowEmptyArchive: true

    // The build trees of the module and all downstreams are large
    if (!opts.keepWorkspace)
      deleteDir()
  }
}


// Records compiler and cmake warnings from the build log of a configuration.
// The paths in the container are mapped to the workspace first.
// Requires the Warnings Next Generation plugin.
def recordWarnings(String name, String nvccParser) {
  String log = "build-${name}.log"
  sh "test -f ${log} && sed -i \"s#/build/#\$WORKSPACE/#g\" ${log}; true"
  // gcc and clang warnings look the same, use the parser of the compiler used
  boolean isClang = sh(returnStatus: true,
                       script: "grep -q 'CXX compiler identification is Clang' ${log}") == 0
  List tools = [isClang ? clang(pattern: log) : gcc(pattern: log),
                cmake(pattern: log)]
  if (nvccParser)
    tools << groovyScript(parserId: nvccParser, pattern: log)
  recordIssues id: "warnings-${name}", name: "Warnings (${name})",
               aggregatingResults: true, enabledForFailure: true,
               skipPublishingChecks: true, tools: tools
}


// Emails failed and unstable builds of the given branches to the authors of
// the changes since the last successful build, and to the default recipients
// of the Extended E-mail Notification configuration
def notifyFailure(List branches) {
  String result = currentBuild.currentResult
  if (env.CHANGE_ID || !branches.contains(env.BRANCH_NAME) || !['FAILURE', 'UNSTABLE'].contains(result))
    return
  emailext to: '$DEFAULT_RECIPIENTS',
           recipientProviders: [culprits()],
           subject: "${env.JOB_NAME} #${env.BUILD_NUMBER}: ${result}",
           body: "${env.JOB_NAME} #${env.BUILD_NUMBER} on ${env.BRANCH_NAME}: ${result}\n\n${env.BUILD_URL}"
}


// Downstreams are built unless the comment says otherwise,
// and always for run triggers
@NonCPS
String addDownstreams(String comment, boolean downstreams) {
  String result = comment ?: ''
  if (downstreams && !result.contains('no downstreams'))
    result += ' with downstreams'
  if ((result =~ /(?i) run /).find())
    result += ' with downstreams'
  return result
}


// Decides which configurations the configuration specific triggers run in.
// Mirrors handle-run-trigger.sh, update-opm-tests.sh and the wheels scripts.
@NonCPS
Map routeTriggers(String comment, List names) {
  Map route = [updateData: (comment =~ /(?i)update_data/).find(),
               wheels: (comment =~ /(?i)wheels/).find(),
               run: (comment =~ /(?i) run /).find(),
               error: null]
  if ((route.updateData || route.wheels || route.run) && !names.contains('default'))
    route.error = 'update_data, wheels and run need the default configuration'
  return route
}


// Removes the configuration specific triggers from the other configurations
@NonCPS
String commentForConfig(String comment, String name) {
  if (name == 'default')
    return comment
  return comment.replaceAll(/(?i)update_data/, '').replaceAll(/(?i) run /, ' ')
}


// The name of the repository the multibranch job builds
String repositoryName() {
  String url = scm.userRemoteConfigs[0].url
  return url.tokenize('/:')[-1] - ~/\.git$/
}
