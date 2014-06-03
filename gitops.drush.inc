<?php
/**
 * @file
 */

include_once 'includes/gitops.git.inc';
include_once 'includes/gitops.persistrepos.inc';
include_once 'includes/gitops.scanrepos.inc';

/**
 * Implements hook_drush_init().
 */
function gitops_drush_init() {
  // Check and ensure that Drush DevTools exists as it is a requirement
  // for this Drush GitOps package.
  if (!defined('DEVTOOLS_VERSION')) {
    drush_set_error('DRUSH_GITOPS: Missing required installed Drush package Drush DevTools. Available: http://github.com/amcgowanca/drush_devtools.');
  }
}

/**
 * Implements hook_drush_command().
 */
function gitops_drush_command() {
  $commands = array();
  $commands['gitops-init'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => 'Initialize the working directory structure for builds and releases.',
    'arguments' => array(
      'vhost_dir' => 'The directory path to the site root of the vhost.',
    ),
    'required arguments' => 0,
    'options' => array(
      'builds_dir' => 'The directory path to the build destination.',
      'builds_repo_dir' => 'The directory path to the headless builds repo.',
    ),
    'aliases' => array('goinit'),
  );
  $commands['gitops-build'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => 'Run a build of the site, commit the result of the build, and push to the builds repo.',
    'arguments' => array(
      'build_file' => 'The file path to the build make file used by drush make, or the file in the repository to download if using the Github download strategy.',
    ),
    'required arguments' => 1,
    'options' => array(
      'vhost_dir' => 'The directory path to the site root of the vhost.',
      'github_user' => 'The Github username.',
      'github_token' => 'The Github user access token.',
      'github_project' => 'The Github project/repository name.',
      'github_ref' => 'The repository ref to retrieve the file from (branch, tag).',
    ),
    'aliases' => array('gobuild'),
  );
  $commands['gitops-clone'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => 'Clone the buildsrepo, setup component repos.',
    'arguments' => array(
      'remote' => 'The remote repository address of the builds repo.',
      'name' => 'The name of the local directory to clone into.',
      'makefile' => 'The path to core make file that built the site.'
    ),
    'required arguments' => 2,
    'options' => array(
      'branch' => 'The branch to pull down and merge.',
    ),
    'aliases' => array('goclone'),
  );
  $commands['gitops-pull'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'description' => 'Pull the latest build from the buildsrepo, make sure component project repos are replaced in the process',
    'arguments' => array(
      'remote' => 'The remote repository address of the builds repo.',
      'makefile' => 'The path to core make file that built the site.'
    ),
    'required arguments' => 2,
    'options' => array(
      'branch' => 'The branch to pull down and merge.',
    ),
    'aliases' => array('gopull'),
  );
  return $commands;
}

/**
 * Implements hook_drush_help().
 */
function gitops_drush_help($section) {
  switch ($section) {
    case 'meta:gitops:title':
      return dt('GitOps');
  }
}

/**
 * Initializes a new working directory structure and project for GitOps.
 *
 * Creates a directory for building the result of the site build into. Creates
 * a directory for storing a headless repo of the site builds (this is a
 * separate repo that only tracks site builds, separate from your working
 * repo/s that contain your individual install profiles and other projects).
 * Creates the headless repo. Initialize repositories in builds directory and
 * vhost.
 */
function drush_gitops_init($vhost_dir = FALSE) {

  // Guess dirs if not provided. Set the default vhost_dir to the directory
  // command was called from. Set the default builds dirs to one directory above
  // vhost dir using current dir (sitename) as part of directory naming schema.
  if (!empty($vhost_dir) && '.' !== $vhost_dir) {

    $vhost_dir_realpath = realpath($vhost_dir);

    if (!is_dir($vhost_dir_realpath)) {

      if (file_exists($vhost_dir)) {

        drush_set_error(dt("The site/vhost directory namespace is already taken by another file. Remove the file or run again with a different site/vhost path."));

        return;

      }
      else {

        drush_mkdir($vhost_dir, TRUE);
        $vhost_dir = realpath($vhost_dir);

      }

    }
    else {

      $vhost_dir = $vhost_dir_realpath;

    }

  }
  else {

    $vhost_dir = drush_cwd();
    // Prompt user to ensure they're in the right directory.
    $confirmation = drush_confirm(dt('You didn\'t pass the site directory parameter, so gitops-init assumes you\'re running it from the directory your vhost/site is located in. Run gitops-init with the current directory as your site/vhost directory: @vhost_dir?', array('@vhost_dir' => $vhost_dir)));

    if (!$confirmation) {
      return;
    }

  }

  $builds_dir = drush_get_option('builds_dir', FALSE);
  $builds_repo_dir = drush_get_option('builds_repo_dir', FALSE);

  $builds_dir = realpath($builds_dir);
  $builds_repo_dir = realpath($builds_repo_dir);

  if (!$builds_dir || !$builds_repo_dir) {

    $vhost_parts = explode('/', $vhost_dir);
    $vhost_name = array_pop($vhost_parts);

    if (!$builds_dir) {

      $builds_dir = implode('/', $vhost_parts) . '/' . $vhost_name . '_build';

    }

    if (!$builds_repo_dir) {

      $builds_repo_dir = implode('/', $vhost_parts) . '/' . $vhost_name . '_builds_repo';

    }

  }

  // Handle input errors to catch mistakes before they cause problems.

  if (is_dir($builds_repo_dir)) {

    // Buildsrepo directory already exists indicating that this site has already
    // been setup. Get out of here so that it's not overwritten accidentally.
    drush_set_error(dt("This site already exists at $builds_repo_dir. Manually remove this directory or pass a specific builds repo directory to gitops-init."));

    return;

  }

  if (is_dir($builds_dir)) {

    // Builds directory already exists indicating that this site has already
    // been setup. Get out of here so that it's not overwritten accidentally.
    drush_set_error(dt("This site already exists at $builds_dir. Manually remove this directory or pass a specific builds directory to gitops-init."));

    return;

  }

  // Setup builds repo directory structure and create headless repo.

  // Create the builds repo directory.
  drush_mkdir($builds_repo_dir, TRUE);
  chdir($builds_repo_dir);
  // Initialize the headless repo.
  $command = 'git init --bare';
  drush_shell_exec($command);
  $command = 'git config receive.denynonfastforwards true';
  drush_shell_exec($command);
  $command = 'git config drushgitops.builddir %s';
  drush_shell_exec($command, $builds_dir);
  $command = 'git config drushgitops.buildsrepodir %s';
  drush_shell_exec($command, $builds_repo_dir);

  // drush_print($builds_dir);
  // return;

  // Setup directory structure and create build repo for the builds destination.
  drush_mkdir($builds_dir, TRUE);
  chdir($builds_dir);

  $command = 'git init';
  drush_shell_exec($command);

  // Create .gitignore.
  file_put_contents('.gitignore', _drush_gitops_gitignore());

  // Initial commit, new branch, push to headless.
  $command = 'git add .';
  drush_shell_exec($command);
  $command = 'git commit -m "Initial commit, add .gitignore."';
  drush_shell_exec($command);
  $command = 'git branch -m 7.x-1.x-builds';
  drush_shell_exec($command);
  // See if this can be eliminated by changing order of steps.
  // $command = 'git branch -D master';
  // drush_shell_exec($command);
  $command = 'git remote add buildsrepo %s';
  drush_shell_exec($command, $builds_repo_dir);
  $command = 'git push buildsrepo 7.x-1.x-builds';
  drush_shell_exec($command);

  // Setup site repo.

  chdir($vhost_dir);

  $command = 'git init';
  drush_shell_exec($command);
  $command = 'git remote add buildsrepo %s';
  drush_shell_exec($command, $builds_repo_dir);
  $command = 'git pull buildsrepo 7.x-1.x-builds';
  drush_shell_exec($command);
  $command = 'git branch -m 7.x-1.x-builds';
  drush_shell_exec($command);


}

/**
 * Initializes a new working directory structure and project for GitOps.
 *
 * Creates a directory for building the result of the site build into. Creates
 * a directory for storing a headless repo of the site builds (this is a
 * separate repo that only tracks site builds, separate from your working
 * repo/s that contain your individual install profiles and other projects).
 * Creates the headless repo. Initialize repositories in builds directory and
 * vhost.
 */
function drush_gitops_build($build_file = FALSE) {

  // If the vhost directory is not specified, use the current working directory.
  // Otherwise we will attempt to retrieve the absolute path and ensure it
  // is actually a directory prior to continuing.
  $vhost_dir = drush_get_option('vhost_dir', FALSE);

  if (!empty($vhost_dir) && '.' !== $vhost_dir) {

    $vhost_dir = realpath($vhost_dir);

    if (!is_dir($vhost_dir)) {
      return drush_log(dt('The specified directory is not a directory and/or does not exist.'), 'error');
    }

  }
  else {

    $vhost_dir = drush_locate_root(drush_cwd());

    if (!is_dir($vhost_dir)) {

      $vhost_dir = drush_cwd();
      // Prompt user to ensure they're in the right directory.
      $confirmation = drush_confirm(dt('You didn\'t pass the site directory parameter, so gitops-build assumes you\'re running it from the directory you want to deploy to. Run the build and deploy to this directory: @vhost_dir?', array('@vhost_dir' => $vhost_dir)));

      if (!$confirmation) {
        return;
      }

    }

  }

  // Get the build file and download to temporary directory.

  // Store the file name in case we need to download it using the Github
  // strategy later.
  $original_build_file = $build_file;

  // Download file from remote location if remote, copies contents of file and
  // creates clone in temp dir if local.
  $build_file = devtools_download_file($build_file);

  if (!$build_file) {

    $github_credentials = array(
      '%user' => drush_get_option('github_user', FALSE),
      '%token' => drush_get_option('github_token', FALSE),
      '%project' => drush_get_option('github_project', FALSE),
      '%file' => $original_build_file,
      '%ref' => drush_get_option('github_ref', FALSE),
    );

    // Make sure all the credentials are present.
    foreach ($github_credentials as $credential => $value) {

      if (empty($value)) {

        drush_set_error(dt("No build file was specified using the --build_file option and one or more Github properties are missing, can't proceed (github_user, github_token, github_project, github_file, github_ref)."));

        return;

      }

    }

    $build_file = _drush_gitops_download_private_github_file($github_credentials);

  }


  // Run build.

  chdir($vhost_dir);

  $command = 'git config --get remote.buildsrepo.url';
  drush_shell_exec($command);
  $builds_repo_dir = drush_shell_exec_output();
  $builds_repo_dir = $builds_repo_dir[0];

  // Handle input errors to catch mistakes before they cause problems.

  if (!is_dir($builds_repo_dir)) {

    // Buildsrepo directory doesn't exist indicating that gitops_init hasn't
    // been run.
    drush_set_error(dt("The builds repo directory doesn't exist. Run `drush gitops-init` to setup structure or otherwise determine why this directory is not present."));

    return;

  }

  chdir($builds_repo_dir);

  $command = 'git config drushgitops.builddir';
  drush_shell_exec($command);
  $builds_dir = drush_shell_exec_output();
  $builds_dir = $builds_dir[0];

  if (is_dir($builds_dir)) {

    drush_delete_dir($builds_dir, TRUE);

  }

  drush_set_option('no-gitinfofile', TRUE);
  drush_set_option('no-cache', TRUE);
  drush_set_option('strict', FALSE);
  drush_invoke('make', array($build_file, $builds_dir));

  if (!is_dir($builds_dir)) {

    // If directory doesn't exist then the build failed and we need to get out
    // of here.
    drush_set_error(dt("Site directory doesn't exist, which likely means the build failed, check for errors."));

    return;

  }

  drush_print(dt("Build succeeded, proceed..."));


  // Push build result to buildsrepo.

  chdir($builds_dir);

  // Remove all the gitignores
  // TODO: Should this be done in php instead?
  $command = 'find . -name ".gitignore" -print0 | xargs -0 rm';
  drush_shell_exec($command);

  // Create repo.
  $command = 'git init';
  drush_shell_exec($command);
  $command = 'git remote add buildsrepo %s';
  drush_shell_exec($command, $builds_repo_dir);
  $command = 'git fetch buildsrepo';
  drush_shell_exec($command);
  // Add existing repo gitignore.
  $command = 'git checkout buildsrepo/7.x-1.x-builds .gitignore';
  drush_shell_exec($command);
  $command = 'git add .';
  drush_shell_exec($command);

  $tag = gmdate('Y-m-d\THi');

  // TODO: Is there more contextual information we can provide beyond just a
  //       timestamp to describe this build?
  $command = 'git commit -m "Build %s"';
  drush_shell_exec($command, $tag);
  $command = 'git checkout -b 7.x-1.x-builds buildsrepo/7.x-1.x-builds';
  drush_shell_exec($command);
  $command = 'git rev-list HEAD --count';
  drush_shell_exec($command);
  $commit_count = drush_shell_exec_output();

  if ((int) $commit_count <= 1) {

    // At this point, master and 7.x-1.x-builds share no common history/ancestor.
    // As a result, it's not so simple to merge or rebase them as both methods
    // either tend to throw errors or create merge commits that result in a really
    // ugly repo history. Instead, use cherry-pick to apply the latest build
    // commit to the upstream history, using the 'theirs' option to instruct it to
    // apply the new build on top of upstream no matter what. Also use
    // 'keep-redundant-commits' to ensure that if the new build does not change
    // any code from the previous build, a commit is still recorded in the repo
    // history so that it is an accurate representation of the build history.
    $command = 'git cherry-pick --keep-redundant-commits --strategy-option=theirs master~0';

  }
  else {

    $command = 'git cherry-pick --keep-redundant-commits --strategy=subtree master~0';

  }

  drush_shell_exec($command);
  $command = 'git push buildsrepo 7.x-1.x-builds';
  drush_shell_exec($command);
  // TODO: Determine if tags should actually be automatically created. There
  //       could be an argument that they're redundant and unnecessary fluff.
  //       Alternatively, create an option for the command to auto-create tags
  //       on each build.
  $command = 'git tag %s';
  drush_shell_exec($command, $tag);
  $command = 'git push buildsrepo --tags';
  drush_shell_exec($command);


  // Deploy new build to vhost.

  chdir($vhost_dir);

  $command = 'git fetch buildsrepo';
  drush_shell_exec($command, $tag);
  $command = 'git reset --hard buildsrepo/7.x-1.x-builds';
  drush_shell_exec($command, $tag);


}

/**
 * Clones the buildsrepo to setup the site files, initializes component repos.
 *
 * This command clones the latest build from the builds repo and then identifies
 * individual site components and sets up their working copy repos.
 *
 * @param string $remote
 *   The path to the git remote buildsrepo.
 * @param string $name
 *   The name of the resultant site root directory.
 * @param string $makefile
 *   The path to the site's main make file used to build the entire site.
 */
function drush_gitops_clone($remote, $name, $makefile) {

  // Clone repo.
  $command = 'git clone %s %s';
  drush_shell_exec($command, $remote, $name);

  $site_root = realpath($name);

  if (!chdir($site_root)) {

    drush_set_error(dt('The clone didn\'t work and the site root was not created. Check to make sure the repo path/address is correct and there are no permissions or access issues.'));

    return;

  }

  $branch = drush_get_option('branch', '7.x-1.x-builds');

  // Checkout latest build.
  $command = 'git reset --hard origin/%s';
  drush_shell_exec($command, $branch);

  drush_delete_dir('.git');
  if (drush_file_not_empty('.gitignore')) {
    unlink('.gitignore');
  }

  $makefile = realpath($makefile);

  $info = make_parse_info_file($makefile);

  $instance = GitopsPersistRepos::getInstance();
  $instance->findRepos($info);
  $instance->initRepos($site_root);

  drush_print(dt('You\'re setup and ready to begin development. However, you\'ll likely still need to install the site using drush site-install or the http site install interface.'));

}

/**
 * Integrates the latest upstream build from the buildsrepo into the site.
 *
 * This command pulls the latest build from the builds repo and attempts to
 * preserve or setup individual component projects inside the site with their
 * component repositories. It does this by first identifying any projects in the
 * site's make files that are being pulled in via git repositories. It
 * moves those component repositories to a temporary directory while it
 * initializes a site-wide repository in the Drupal root, fetches the latest
 * build from the buildsrepo, checks out that build, and subsequently deletes
 * the newly initialized repository. This, in-effect, updates the site files to
 * match the latest build. It then replaces the individual component repos by
 * moving them back from their temporary directories into their correct site
 * position.
 *
 * @param string $remote
 *   The path to the git remote buildsrepo.
 * @param string $makefile
 *   The path to the site's main make file used to build the entire site.
 */
function drush_gitops_pull($remote, $makefile) {

  $root = drush_locate_root(drush_cwd());

  if (empty($root)) {

    drush_set_error(dt("No Drupal site could be found, please run within site."));

    return;

  }

  $makefile = realpath($makefile);

  $info = make_parse_info_file($makefile);

  $instance = GitopsPersistRepos::getInstance();
  $instance->findRepos($info);
  $instance->moveRepos();


  // Fetch and reset latest build.

  chdir($root);

  /*if (is_dir('.git')) {

    drush_set_error(dt('A Drupal root level repo already exists. This is not what gitops-pull expects.'));

  }*/

  // Create repo.
  $command = 'git init';
  drush_shell_exec($command);
  $command = 'git remote add buildsrepo %s';
  drush_shell_exec($command, $remote);
  $command = 'git fetch buildsrepo';
  drush_shell_exec($command);

  $branch = drush_get_option('branch', '7.x-1.x-builds');

  // Checkout latest build.
  $command = 'git reset --hard buildsrepo/%s';
  drush_shell_exec($command, $branch);

  // Get rid of repo.
  drush_delete_dir('.git', TRUE);
  if (drush_file_not_empty('.gitignore')) {
    unlink('.gitignore');
  }


  // Replace the moved_projets.

  $instance->replaceRepos();


}
