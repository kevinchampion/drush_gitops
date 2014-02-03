<?php
/**
 * @file
 */

/**
 * Implements hook_drush_command().
 */
function devops_drush_command() {
  $commands = array();
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
