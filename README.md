![Lupe](https://repository-images.githubusercontent.com/531184514/f0ec9a0e-bd24-4625-92e6-e4d25e6b0447)

Lupe - Lua apps and packages as directory trees
===============================================

Quickstart
----------

1. Create a folder/directory
2. enter in it
3. run `lupe init`
4. explore the structure created inside the folder and
   edit the files as you want to.


The standard Lupe file system
------------------------------
```
yourpkg
 │
 ├─ bin  <··· The place with your Lua scripts
 │   ├─ main.lua
 │   ╰─ somecommand.lua
 │
 ├─ etc/  <··· Where the confs of your package are
 │   ├─ template.tpl
 │   ╰─ anything.jpg
 │
 ├─ lib/  <··· Strictly Lua modules
 │   ├─ a.lua ··· got as require("yourpkg.a")
 │   ╰─ b.lua ... git as require("yourpkg.b")
 │
 ├─ dep/  <··· Your dependencies go here
 │   ╰─ otherpkg/
 │       ├─ etc/
 │       ╰─ lib/
 │
 ╰ lupe
```

* `etc` folder is the place where you can add yout project assets. For a console
program you can have files containing colorschemes, or data skeleton, or any
other configuration you want to. Like in the Unix etc directory. If the project
is related to a website, here you can put your templates, css, js etc.
* `lib` folder contains the modules of your package. The modules contained in this
folder have precedence over the instaled out of lupe folder. The way you see the
structure in this folder is the way you can require them (the simple Lua way)
* `dep` stands for dependencies. Here is where your dependencies are installed.

The Lupe root file
-------------------

This file marks the root of the Lupe system structure, and contains important
information about the system:

* `lua_version = LIST` inform for which Lua versions it is intended to work.
When running the Lupe it will follow this list in order finding the correct
Lua version for your application. Ex: `lua_version={"5.4","5.3"}`

* `dep = INDEX` is a table where the keys are the name of the package and the
value is a string or a table. If it is only a string, on update it will download
and update the git resository at the url. If it is a table Lupe will consider
the information provided there to choose the right Git reference (branch or tag)
as well as if it shouldnt do a shallow clone. Ex:

```lua
dep = {
    extpkg = "https://gitsite/user/extpkg",
    otherext = { "https://gitsite/user/otherext", dir="src/lua", ref="dev" },
}
```

Executable Lupe folder
-----------------------

As you see, when you execute `lupe init` a `./bin/main.lua` file is created.
If anywhere from your system you run `lupe /your/proj/dir` it will look
for the `/your/proj/dir/bin/main.lua` and execute it.

You may also link the `/your/proj/dir/bin/main.lua` to your system `$PATH`, eg:

`ln -sf /your/proj/dir/bin/main.lua /usr/local/bin/yourproj`

Now, when you run `yourproj` from anywhere on your system it will correctly
execute the `main.lua` file resolving all the inner dependencies of your Lupe
folder.

Creating more than one Lua executable script inside Lupe
---------------------------------------------------------

You can add multiple Lua executable scripts inside the `./bin` folder under
your lupe project. They just need to be executable (with `chmod +x`) and
has the hasbang on the first line like `#!/usr/bin/env lupe`.


Why don't use only Luarocks?
----------------------------

Luarocks is great for installing rocks in system or on a folder. It really
rocks when writting packages in C and compiled for Lua.

But things become complex when you want to include private dependencies or
develop multiple packages at the same time... Well, I won't describe all
the process here, but feel free to try and discover the complexity.

Here comes the idea of Lupe:

- Provide a simple way to quickstart a project with Lua only files.
- Resolve the path in a way that you can just drop your project folder
anywhere and start using, with no need to install it on a fixed folder
or handle manually the `package.path`.
- Provide a finder for the right name of the Lua binary in different
systems.
- Allow the development of multiple packages: you can configure the
`deps` entry to not do a shallow clone of a git repository. So you
can write your code and improve other packages as you go.
- You don't need to update a rockspec everytime you create a Lua file,
neither rebuild your project.
- You can use code from repositories that don't have a rockspec.

Next steps
----------

1. Allow more isolated usage with a option to block external paths
from `package.path` and wrap Luarocks call to install under the Lupe
folder.
2. Dependency resolution for packages installed from a Git repository.


