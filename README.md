# Drush DevOps

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



