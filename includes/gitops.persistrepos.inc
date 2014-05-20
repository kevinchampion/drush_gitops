<?php
/**
 * @file
 * Drush preserve git processing classes.
 */

/*******************************************************************************
 * TODO
 *
 * - consider stashing any local repo changes and checking out a new temporary
 *   branch to accommodate the build changes if there are any, this would be a
 *   helper and probably should be optional
 *
 ******************************************************************************/

/**
 * The base project class.
 */
class GitopsPersistRepos {

  /**
   * Keep track of instances.
   *
   * @see GitopsPersistRepos::getInstance()
   */
  protected static $self = array();

  /**
   * TRUE if make() has been called, otherwise FALSE.
   */
  protected $found_repos = array();

  /**
   * Set attributes and retrieve project information.
   */
  protected function __construct() {

  }

  /**
   * Get an instance for the type and project.
   *
   * @param string $type
   *   Type of project: core, library, module, profile, or translation.
   * @param array $project
   *   Project information.
   *
   * @return mixed
   *   An instance for the project or FALSE if invalid type.
   */
  public static function getInstance() {
    return new GitopsPersistRepos();
  }

  public function getFoundRepos() {
    return $this->found_repos;
  }

  /**
   * Find repos obtain list of projects needing to be moved and replaced.
   *
   * @param array $info
   *   Parsed .make file.
   */
  public function findRepos($info) {

    if (!isset($info['projects'])) {
      return TRUE;
    }

    foreach ($info['projects'] as $key => $project) {

      $project['name'] = $key;
      $project['parent'] = $info['parent'];

      $instance = GitopsScanRepos::getInstance($project['type'], $project, $this);

      $instance->findRepos();

      $result = $instance->getFoundProjects();

      if (!empty($result)) {

        $this->found_repos[] = $result;

      }

    }

  }

  /**
   * Move all found repos from their source to a temporary directory.
   */
  public function moveRepos() {

    foreach ($this->found_repos as $repo) {

      if ($repo['repo_exists']) {

        drush_move_dir($repo['src'] . '/.git', $repo['dest'] . '/.git', TRUE);
        if (drush_file_not_empty($repo['src'] . '/.gitignore')) {
          rename($repo['src'] . '/.gitignore', $repo['dest'] . '/.gitignore');
        }

        // DEBUGGING ***remove***
        if (is_dir($repo['dest'] . '/.git')) {

          drush_print_r($repo['project']['name'] . ' moved to tmp');

        }
        else {

          drush_print_r($repo['project']['name'] . ' failed to move to tmp');

        }

        if (!is_dir($repo['src'] . '/.git')) {

          drush_print_r($repo['project']['name'] . ' no longer located in site');

        }
        else {

          drush_print_r($repo['project']['name'] . ' still in site');

        }

      }

    }

  }

  /**
   * Replace all found repos from their temporary location to the project.
   */
  public function replaceRepos() {

    foreach ($this->found_repos as $repo) {

      if ($repo['repo_exists']) {

        drush_move_dir($repo['dest'] . '/.git', $repo['src'] . '/.git', FALSE);
        if (drush_file_not_empty($repo['dest'] . '/.gitignore')) {
          rename($repo['dest'] . '/.gitignore', $repo['src'] . '/.gitignore');
        }

        // TODO: What about .gitignores?
        // git checkout HEAD^ .gitignore

        // DEBUGGING ***remove***
        if (is_dir($repo['src'] . '/.git')) {

          drush_print_r($repo['project']['name'] . ' returned from tmp');

        }
        else {

          drush_print_r($repo['project']['name'] . ' failed to return from tmp');

        }

        if (is_dir($repo['dest'] . '/.git')) {

          drush_print_r($repo['project']['name'] . ' still in tmp');

        }
        else {

          drush_print_r($repo['project']['name'] . ' no longer located in tmp');

        }

      }

    }

  }

  /**
   * Replace all found repos from their temporary location to the project.
   */
  public function initRepos($root) {

    if (!chdir($root)) {

      // TODO: Error message.

      return;
    }

    foreach ($this->found_repos as $repo) {

      drush_print_r($repo);
      drush_print_r($root . $repo['src']);

      if (chdir($root . $repo['src'])) {

        // Create repo.
        $command = 'git init';
        drush_shell_exec($command);
        $command = 'git remote add origin %s';
        drush_shell_exec($command, $repo['project']['download']['url']);
        $command = 'git fetch origin';
        drush_shell_exec($command);
        $command = 'git reset --hard origin/%s';
        // TODO: What if this is a tag ref instead of a branch? What about a
        //       commit hash?
        drush_shell_exec($command, $repo['project']['download']['branch']);
        $command = 'git branch -m master %s';
        drush_shell_exec($command, $repo['project']['download']['branch']);

      }

    }

  }


}
