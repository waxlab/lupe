![Lupe](https://repository-images.githubusercontent.com/531184514/5f2c2dc3-2d34-41d7-9fce-681a68a66ff1)

Lupe - Lua Package Environment
==============================

Lupe gives you a simple way to develop and distribute your projects.
It is made by:

* Giving you a simple folder structure easy to reason about.
* Avoiding boilerplate code for package searching or path settings
* Encapsulating your project and dependencies under a single folder
* Enabling Lua interpreter binary detection in a system independent way
* Allowing you to use Lua packages provided by unconventional ways (and places)


Quickstart
----------

1. Create a folder/directory and switch to it.
2. run `lupe init`
3. explore the structure created inside the folder and
   edit the files as you want to.


The standard Lupe file system
------------------------------

See below a fictitious folder structure for a project created with Lupe:
```

SolarSystem
 |
 +- bin  <··· The place with your Lua scripts
 |  |
 |  +- main.lua
 |  |
 |  +- sun.lua
 |
 |
 +- etc/  <··· Where the confs of your package are
 |  |
 |  +- template.tpl
 |  |
 |  +- anything.jpg
 |
 |
 +- lib/  <··· Strictly Lua modules
 |  |
 |  +- asteroid.lua ··· got as require("asteroid")
 |  |
 |  +- planet.lua ··· got as require("planet")
 |  |
 |  +- satellite.lua ... git as require("planet.satellite")
 |  |
 |  +- comet.lua ... git as require("comet")
 |
 |
 +- deps/  <··· Your dependencies go here
 |  |
 |  +- .rocks/ <··········· directory used to install rocks via Luarocks
 |  |
 |  +- starchem/
 |  |
 |  +- lib/
 |     |
 |     +- starchem/
 |        |
 |        +- hydrogen.lua ··· got as require("starchem.hydrogen")
 |        |
 |        +- helium.lua ····· git as require("starchem.helium")
 |
 |
 +- starfeats/
 |  |
 |  +- src/
 |     |
 |     +- spots.lua ······ got as require("starfeats.spots")
 |     |
 |     +- flares.lua ····· git as require("starfeats.flares")
 |
 |
 +- luperc.lua
```

* `etc` folder is the place where you can add yout project assets. For a console
program you can have files containing colorschemes, or data skeleton, or any
other configuration you want to. Like in the Unix etc directory. If the project
is related to a website, here you can put your templates, css, js etc.
* `lib` folder contains the modules of your package. Think of the modules found
here being the focus of your project.
* `deps` stands for dependencies. You can just drop other Lua packages here from
other projects that you want to use as dependencies. See **dependencies**
section to understand how to work here.
* `luperc.lua` stands for **Lua Package Environment Resources Config** and is
a plain declarative Lua file where you define the options of your project.

The Lupe Config File - luperc.lua
---------------------------------

For the above fictitious project tree we would have a `luperc.lua` file containing
something like below:

```Lua
lua = { "5.2", "5.3" }
deps = {
    starchem = "starchem/lib",
    starfeats = { "starfeats/src", cutoff=true }
}
rocks = {
    "dkjson",
    "curl",
}
```

As you see it is a plain Lua with variables and tables only.

1. `lua` contains a list of Lua versions allowed to run the project. Currently
Lupe supports from Lua 5.1 to 5.4. The `lupe` command looks at this configuration
when running a Lua script from the `bin` folder to find the Lua binary needed
to run your project. Observe that the value precedence are left-to-right, i.e.
if you use `lua = {"5.4", "5.2"}` it will look for Lua 5.4 first and use it.
If Lua 5.4 was not found, then it will continue looking for Lua 5.2 and so on.

2. `deps` is a record table (a dictionary) where each key matches the name of
the package as you will use inside your project and the value determines where
under the `./deps` folder it is. You can, by example drop a folder with a
package you wrote for other Lua project. Or you can clone a Git, Subversion,
Mercurial etc. repository under a folder inside `./deps` and give the instruction
on where your project will find the correct files. Lupe allows you to use Lua
code from the most diverse sources here.

By now, a deps entry can be defined in two ways.

    deps = {
        starchem = "starchem/lib",
    }   \______/   \____________/
           |             |
           |             path relative to the ./deps folder
           package name

This indicates that when you use under your Lua script `require "starchem"` the
code will be obtained from `./deps/starchem/lib/starchem/init.lua` or
`./deps/starchem/lib/starchem.lua`.

Observe that in the above example all the package structure is under the `lib`
folder.

Sometimes however, you may want to use a not so well structured project as your
dependency. Suppose you need a dependency called `starfeats` with the modules
`spots` and `flares`. You can require them in Lua with `require "spots"` and
`require "flares"`. But they are found in a project in some git repository named
`StarFt` and with the files `src/spots.lua` and `src/flares.lua`. Even you
`git clone git://.../StartFt ./deps/starfeats` you could'nt require the modules
properly because the directory where modules are is named `src`. In this case
you can set a cutoff, i.e., make Lupe use the package name not looking for a
package dir, but using the directory files as submodules.

    deps = {
       starfeats = { "somedir/src", cutoff=true },
    }  \_______/     \___________/  \_________/
           |               |             use the modules without
           |               |             a named package dir
           |               |
           |               path relative to the ./deps folder
           package name

Observe that the path relative to the deps folder can have any name. But for the
sake of good structuring, it is better use a folder refering clearly to the
package name.

3. `rocks` entry is a list containing the Luarocks modules in the same way you
find them in a Luarocks rockspec under the dependencies section. When issuing
`lupe update` it will look at this entry and will run Luarocks using the
`./deps/.luarocks` as the tree and the found Lua version as the target. By now,
it won't reinstall modules found in a reachable tree outside the Lupe folder.


Executable Lupe/Lua file
------------------------

An executable Lupe/Lua file is basically a standard Lua file shebanged with
`#!/usr/bin/env lupe` and with executable attribute. The only requisite to
work is to be included under the `bin` folder of your Lupe directory and have
a `luperc.lua` on the parent directory containing the `lua` entry as explained
above.

You can, then run it from everywhere on your computer specifying the full path,
and automatically will be run with the available Lua interpreter that fits the
version specified.

You can set your `./bin` folder under the Lupe folder as part of the system
`$PATH` variable, make an alias on your shell or even link it simbolically to
a folder under the system `$PATH`. No matter from where the file be called it
will resolve the Lupe directory, understand the `luperc.lua` and find the
dependencies correctly.


Executable Lupe folder
-----------------------

When you execute `cd /proj/dir && lupe init` a `./proj/dir/bin/main.lua`
executable file is created. If anywhere from your system you run 
`lupe /proj/dir`, Lupe will run the `/proj/dir/bin/main.lua` as described above
in the **Executable Lupe/Lua file**


Creating more than one Lua executable script inside Lupe
---------------------------------------------------------

You can add multiple Lua executable scripts inside the `./bin` folder under
your lupe project. They just need to be executable (with `chmod +x`) and
has the hasbang on the first line like `#!/usr/bin/env lupe`.




