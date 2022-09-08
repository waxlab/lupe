Luft - Runnable Packages for Lua
===============================

*Luft* [noun] natural ability to relate, understand and perceive things.
Reasoning power.

Luft tries to relate the best things to allow reasoning power while building
good software in a malleble way. In other term we could say these of Luft:
* Manages pure Lua packages lufthout a centralized repository or index
* Targets Lua files as programming language
* Allows a Luft folder to be a package
* Allows a Luft folder to be a runnable Lua project
* Allows a Luft folder to have dependency tree in itself
* Allows a Luft folder to be the place where code finds its assets
* Doesn't limit a package to be public or have only public packages
* Is a framework to kickstart Lua apps or packages in a scalable way.

Luft consists in:

* the `luft` Lua modules, used to help programmer using The luft folder structure
as the app file system.

* the `luft` binary, that is a runnable Lua script that allows the creation and
management of the Luft folder as an app or package.

* the `luftsh` binary is the folder interpreter that allows that a folder lufth
the file `main.lua` to be runnable.


Usage for a existing project
----------------------------

1. `wiz get GITURL` it will get your project from the repository and resolve
its dependencies.
2. `luft ./` to run your project.


Creating a new project
----------------------

1. `wiz init PKGNAME` to set the current directory as a project. If `PKGNAME`
is not set it tries to use from your directory name (the first word part only).
It will also create a filesystem structure inside that as a template (explained
below).

2. Add your dependencies as submodules `wiz add GITREPO`. It will make a shallow
clone of the `GITREPO` under the `sub` folder and resolve its dependencies.

3. Start coding.


The directory structure
-----------------------

* `self.lua` is just a Lua file filled lufth variables, hashbanged lufth Luft. and
executable. It is used by Luft to locate your project and its meta informations.
The main information is `name =` that contains the package name, and Luft will use
it in its package loader. Its hashbang points to luft, so luft will call the file,
detect its realpath and find the directory structure, calling Lua lufth proper
configurations. You can also create a link of this file (see, link not copy!) to
your `PATH` folder so you can start your project anywhere.

* `src` is where you put the Lua modules of your package. It you have a
`src/rocket.lua` and `name = "mission` on your `self.lua` so you will load this
module as `mission.rocket`.

* `sub` folder is where `wiz add` puts the subpackages.

* `cmd` folder contains the commands for your Luft folder app. If you have a
file named `cmd/launch.lua` then you can run `./self.lua launch`. If you
intend to use your folder app as runnable, you need at least `cmd/init.lua`.

* `files` is where you will put your assets like images, documents, config etc.
related to the project but not the program itself.

