<?php
/**
 * @file
 */


/**
 * Implements hook_drush_init().
 */
function devops_drush_init() {
  // Check and ensure that Drush DevTools exists as it is a requirement
  // for this Drush DevOps package.
  if (!defined('DEVTOOLS_VERSION')) {
    drush_set_error('DRUSH_DEVOPS: Missing required installed Drush package Drush DevTools. Available: http://github.com/amcgowanca/drush_devtools.');
  }
}

/**
 * Implements hook_drush_command().
 */
function devops_drush_command() {
  $commands = array();
  $commands['devops-init'] = array(
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
    'aliases' => array('ops-init, doi'),
  );

  return $commands;
}

/**
 * Implements hook_drush_help().
 */
function devops_drush_help($section) {
  switch ($section) {
    case 'meta:devops:title':
      return dt('Devops');
  }
}

/**
 * Initializes a new working directory structure and project for DevOps.
 *
 * Creates a directory for building the result of the site build into. Creates
 * a directory for storing a headless repo of the site builds (this is a
 * separate repo that only tracks site builds, separate from your working
 * repo/s that contain your individual install profiles and other projects).
 * Creates the headless repo. Initialize repositories in builds directory and
 * vhost.
 */
function drush_devops_init($vhost_dir = FALSE) {

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

  }
  else {

    $vhost_dir = drush_cwd();
    // Prompt user to ensure they're in the right directory.
    $confirmation = drush_confirm(dt('You didn\'t pass the site directory parameter, so devops-init assumes you\'re running it from the directory your vhost/site is located in. Run devops-init with the current directory as your site/vhost directory: @vhost_dir?', array('@vhost_dir' => $vhost_dir)));

    if (!$confirmation) {
      return;
    }

  }

  $builds_dir = drush_get_option('builds_dir', FALSE);
  $builds_repo_dir = drush_get_option('builds_repo_dir', FALSE);

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
    drush_set_error(dt("This site already exists at $builds_repo_dir. Manually remove this directory or pass a specific builds repo directory to devops-init."));

    return;

  }

  if (is_dir($builds_dir)) {

    // Builds directory already exists indicating that this site has already
    // been setup. Get out of here so that it's not overwritten accidentally.
    drush_set_error(dt("This site already exists at $builds_dir. Manually remove this directory or pass a specific builds directory to devops-init."));

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
  $command = 'git config drushdevops.builddir %s';
  drush_shell_exec($command, $builds_dir);
  $command = 'git config drushdevops.buildsrepodir %s';
  drush_shell_exec($command, $builds_repo_dir);

  // drush_print($builds_dir);
  // return;

  // Setup directory structure and create build repo for the builds destination.
  drush_mkdir($builds_dir, TRUE);
  chdir($builds_dir);

  $command = 'git init';
  drush_shell_exec($command);

  // Create .gitignore.
  file_put_contents('.gitignore', _drush_devops_gitignore());

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



/*******************************************************************************
 * Helper functions, should be moved to include files.
 ******************************************************************************/

/**
 * Construct gitignore file contents as a string.
 *
 * @return string
 *   Contents of the gitignore file to be used for the buildsrepo (the result
 *   of the build containing all of the site code).
 */
function _drush_devops_gitignore() {

  $ignores = array(
    '# Ignore configuration files that may contain sensitive information.',
    'sites/*/*settings*.php',
    '!sites/default/default.settings.php',
    '',
    '# Ignore paths that contain user-generated content.',
    'sites/*/files',
    'sites/*/private',
    '',
    '# Ignore radioactivity bootstrap config',
    'sites/all/contrib/radioactivity/radioactivity-bootstrap.cfg.inc',
    'profiles/*/modules/contrib/radioactivity/radioactivity-bootstrap.cfg.inc',
    '',
    '# Ignore contrib modules',
    'sites/all/modules/contrib/ignore',
    '',
    '# APC cache clear',
    'apc_clear.php',
    '',
    '# DRUPAL',
    'CHANGELOG.txt',
    'INSTALL.mysql.txt',
    'INSTALL.pgsql.txt',
    'INSTALL.txt',
    'MAINTAINERS.txt',
    'UPGRADE.txt',
    'INSTALL.sqlite.txt',
    '',
    '# Numerous always-ignore extensions',
    '*.diff',
    '*.err',
    '*.orig',
    '*.log',
    '*.rej',
    '*.swo',
    '*.swp',
    '*.vi',
    '*~',
    '',
    '# OS or Editor folders',
    '.DS_Store',
    '.sass-cache',
    '.cache',
    '.idea',
    '.project',
    '.settings',
    '.tmproj',
    'nbproject',
    'Thumbs.db',
    '.buildpath',
    '',
    '# Folders to ignore',
    '.hg',
    '.svn',
    '.CVS',
    'cgi-bin',
  );

  return implode("\n", $ignores);

}


