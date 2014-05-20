# Drush GitOps

## Scenarios

1. You want to setup an automated build process somewhere so that you can have continuous integration when you structure your sites by combining various components in a make file and only tracking those parts of your site that are unique (install profiles are great for this). You want to set the structure up with the foresight that you'll eventually need a mechanism for deploying the entire site's codebase (the result of the build) to other environments.

    Drush DevOps Init will initialize a directory structure and create a headless repository that you will use for storing and tracking the results of your site builds. The directory structure has three components:

    1. A vhost directory where you can host a full site install.
    2. A build directory which will be destroyed and rebuilt on each build, and where the build changes are tracked and pushed to the headless repo.
    3. A headless repo directory (buildsrepo) which contains and tracks the changes of each build.

2. You want to setup the actual automated build process and utilize the structure setup in devops-init (Scenario #1). You want to run a full site build and then commit the result of the build to the buildsrepo. At the same time, you want to deploy the new build to a vhost (usually your `dev` environment) so that you have continuous integration in an environment and can explore and test the changes.

    Drush DevOps Build will run a full site build into the builds directory, commit the result of the build and push it to the headless buildsrepo, and deploy those changes to the specified vhost.

3. You want to setup a development environment without having the run builds in that environment. This could be for you or another developer, but is primarily oriented around getting started working on a site whose code is structured and tracked as individual components and built from make files.

    Drush DevOps Clone will clone the codebase from the buildsrepo, remove the top-level repository, and initialize the individual working repositories in the components defined (in the make files) to be built from git repos. This allows a developer to get setup and running quickly by retrieving the entire codebase, and subsequently setting up each components working repo.

4. You want to integrate the latest build into a development environment without having to run builds in that environment. This could be for you or another development, but is oriented to enable easy integration with the latest upstream changes without requiring a complete destruction and rebuild of the environment's site.

    Drush DevOps Pull will move each individual component's repo to a temporary directory, pull the latest build from the buildsrepo, remove the top-level repository, and replace the individual component repos from the temporary directory back to their component directory in the site.

## Commands

### devops-init

Sets up folder structure for having a vhost/site directory, a
builds directory for the destination of the result of individual
builds, and a headless repo for storing the results of the builds.

### devops-build

Adds command to download a make file, run a full build of the site
from the make file, commit the result of the build and push it to
the headless buildsrepo, and deploy the new build code to the
site/vhost.

### devops-clone

Clones full site from the buildsrepo, deletes the main site repo,
identifies all projects that should have repos, goes through each
and sets up working copy repo.

### devops-pull

Moves component repos to a temporary directory, initializes drupal
root repo and pulls the latest build from the buildsrepo, removes
drupal root repo, and replaces component repos back in their
correct project directory.

## Requirements & Dependencies

- Drush DevTools

## Deployment Instructions

- Merge code, install utility in your environment.
- Clear drush caches with `drush cache-clear drush`

## Usage/Test Instructions

- Run `drush devops-init` from an empty directory nested inside another empty directory, or pass it the path of an existing empty directory or the name of a directory you'd like it to create to house your site/vhost `drush devops-init my-site`.
- Check to see that three directories exist, the vhost/site, the build destination, and the builds repo. The vhost/site and build destination should both contain .git directory and a .gitigore file and should currently have the 7.x-1.x-builds branch checked out. The builds repo directory should contain a headless repo.
- Run `drush devops-build build-my-site.make` passing in the path to the build file you're building from (can be local file or publicly accessible remote path). Alternatively, run the command with the name of your build file and github details to download the file from a github repo `drush devops-build build-my-site.make --github_user=my-github-username --github_project=my-github-repo-name --github_token=my-github-access-token --github_ref=7.x-1.x`
- Check to see that the build succeeded to run without error and that the site/vhost now contains a fully file populated site. The repo should be checked out to the 7.x-1.x-builds branch and the git log should now show 2 commits in it, the initial commit and a build commit containing the result of the build you just ran.

 ## TODO

  - add documentation
  - split out helper functions into their own files
  - determine if some procedural steps in command should be broken into their
    own functions
  - add logging throughout
  - test (and add if needed) the ability to host the buildsrepo on a remote service like github


  ### Features

  - add ability to download private github file using non-token user
    credentials (may be helpful for flexibility and ease of use in different
    environments)
  x add command/s for working with a built site without having to actually run
    builds in your environment by pulling from the buildsrepo but pushing to
    the install profile or other repo
    - add option to devops-pull to initialize new working copy repo for each
      project rather than move and replace existing repos
    - add option to stash working changes and checkout specific branch to house
      the build result changes
    - add warnings about destruction of local working changes
  x add command (devops-clone) for initializing repos of all git projects as
    found in make files, setup task after cloning a buildsrepo to get a new
    environment setup
  - add command (devops-component-pull) to refresh each project repo with a
    pull from their origin
  - add command (devops deploy) to deploy the latest code from the buildsrepo to another environment


