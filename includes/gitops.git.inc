<?php
/**
 * @file
 * Drush git utility functions.
 */


/**
 * Construct gitignore file contents as a string.
 *
 * @return string
 *   Contents of the gitignore file to be used for the buildsrepo (the result
 *   of the build containing all of the site code).
 */
function _drush_gitops_gitignore() {

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

/**
 * Utility to download a private github file using credentials.
 *
 * @param array $github_credentials
 *   Contains github access credentials and project specifics.
 * @return [type]
 */
function _drush_gitops_download_private_github_file($github_credentials) {

  $url = 'https://api.github.com/repos/%user/%project/contents/%file?access_token=%token&REF=%ref';
  $url = strtr($url, $github_credentials);

  $options = array(
    CURLOPT_URL => $url,
    CURLOPT_USERAGENT => 'Accept: application/vnd.github.v3.raw',
    CURLOPT_RETURNTRANSFER => TRUE,
  );

  $response = _drush_gitops_curl_github($options);

  if (!$response) {

    drush_set_error(dt('Curl failed to download the requisite Github API information to be able to retrieve the build file.'));

    return;

  }

  $repo_info = json_decode($response, TRUE);

  $url = $repo_info['git_url'];
  $url .= '?access_token=%token';
  $url = strtr($url, $github_credentials);

  $options = array(
    CURLOPT_URL => $url,
    CURLOPT_USERAGENT => 'Accept: application/vnd.github.v3.raw',
    CURLOPT_RETURNTRANSFER => TRUE,
  );

  $response = _drush_gitops_curl_github($options);

  if (!$response) {

    drush_set_error(dt('Curl failed to download the requisite Github API information to be able to retrieve the build file.'));

    return;

  }

  $data = json_decode($response);

  $temp_dir = devtools_tempdir();
  $temp_path = $temp_dir . '/' . $github_credentials['%file'];

  file_put_contents($temp_path, base64_decode($data->content));

  return $temp_path;

}

/**
 * Generic curl execution.
 *
 * @param array $options
 *   Curl options to set for executing the request.
 *
 * @return json
 *   File contents as retrieved from Github.
 */
function _drush_gitops_curl_github($options) {

  $ch = curl_init();

  curl_setopt_array($ch, $options);

  if(!$tmp = curl_exec($ch)) {
    curl_close($ch);
    return false;
  }

  if ($errmsg = curl_error($ch) || $tmp == 'Not Found') {
    curl_close($ch);
    return false;
  }

  curl_close($ch);

  return $tmp;

}
