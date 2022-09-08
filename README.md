![Luft](https://repository-images.githubusercontent.com/531184514/2f3d2440-66d9-4e33-b7aa-404d42de8b84)

Luft - Lua apps and packages as directory trees
===============================================

> In Germany *luft* means air.
>
> In Iceland *loft* is related to the sky or heaven.
>
> In England, *lift* is to move upwards.
>
> In Lua, __*Luft*__ is make your code go beyond...

Luft tries to relate the best things to allow reasoning power while building
good software in a malleable way. How it is possible?

* Create a directory that becomes a package and add other packages inside it
as dependencies.
* Make a runnable package tree and link it on the path running as a system command.
* Tuned up require to find the local Lua modules beside the system Lua modules.
* Tuned local requires to find only modules related to current module
* A set of tools to help loading files only from current package.
way.
* If you have the directory and Git you can freely distribute or code,
publicly or privately. The freedom to build your code with no hurry and no mess.

Luft consists in:

* A file named `index.luft` on the root of the Luft tree. It is used as
milestone when navigating through packages and also to identify the package
as runnable.
* An interpreter to be used as `#!/bin/env luft` to be hashbanged on
`index.luft`. It allows the folder to run with the correct Lua version.
* A manager... the self `luft` binary, with abilities to setup and (perhaps)
update a Luft tree.
* The `luft` Lua modules, that enriches the Lua way of requiring modules and
add helpers to allow file loading (any asset of your project) inside the
package.


The Luft file system
-------------------
```
yourpkg
 │ 
 ├─ etc/  <··· Here are your package assets
 │   ├─ template.tpl
 │   ╰─ anything.jpg
 │
 ├─ lib/  <··· Here comes your package modules
 │   ├─ a.lua ··· to be required as yourpkg.a
 │   ╰─ b.lua ... to be required as yourpkg.b
 │
 ├─ sub/  <··· Your dependencies go here
 │   ╰─ otherpkg/
 │       ├─ etc/
 │       ├─ lib/
 │       ├─ sub/
 │       ╰─ index.luft
 │
 ╰ index.luft
```
* `etc` folder is the place where you can add yout project assets. For a console
program you can have files containing colorschemes, or data skeleton, or any
other configuration you want to. Like in the Unix etc directory. If the project
is related to a website, here you can put your templates, css, js etc.
* `lib` folder contains the modules of your package. These modules can be required
using the package folder name (in case yourpkg) or relatively to itself when
your package is intended or use inside other packages. See more in the documentation
about `require.self()`.
* `sub` is where you can put the dependecies of your package. The name of the folder
is the name that will be used when doing requires. To require from these dependencies
you should use the `require.sub()`

The functions `require.self()` and `require.sub()` gives a high control of what
you really want to require. You can use two versions of same packages in your
project.
