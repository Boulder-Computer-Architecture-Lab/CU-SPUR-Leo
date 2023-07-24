# status
currently, compiling and testing works with ``gcc 13.1.1`` (and associated ``glibc``) for ``amd64``.

tested gem5 host architectures:
- ``aarch64``
- ``amd64``

the base binary without command-line options has been tested on all gem5 builds.


# notes on gem5 compatibility

your gem5 configuration may not support passing command line options to its workload by default.

there are two ways of working around this:
- re-compilation with macros set
- modification of the gem5 configuration.

## re-compilation with macros

in ``defines.h``, under a section labelled ``command defaults``, you can adjust the default settings of program parameters.

of note here is ``TESTS``. by default, it should be set to 0, which enables all tests. to select specific tests, list the desired tests afterwards separated by ``|``, i.e.
```c
#define TESTS PHT_CODE | BTB_CODE
```

then, recompile, using ``make`` in the ``/new`` directory.

## modification of gem5 configuration

these details are more relevant for the ``configs/learning_gem5/part1/two_level.py`` configuration, but are applicable to other cases.

to accept command line arguments:
- find some means of accepting the arguments to begin with
- modify the ``process.cmd`` parametre of the ``Process()`` object that is used as a workload to contain the arguments

for example, for the ``two_level.py`` configuration, i added the following line:
```python
SimpleOpts.add_option("--bin-opts")
```
under the other ``SimpleOpts.add_option`` for ``"binary"``. 

then, i modified the line that sets ``process.cmd`` to the following:

```python
process.cmd = [args.binary] + args.bin_opts.split(" ")
```

this enables command-line arguments to be passed in like this:
```shell
build/x86/gem5.opt configs/learning_gem5/part1/two_level.py <other options> --bin-opts="-t -f" <binary>
```
(replace ``"-t -f"`` with whatever you're using)

if you're not using the ``SimpleOpts`` module in ``configs/common``, then adding options may require a little more work with ``argparse`` in the configuration file. also see ``configs/example/se.py`` for a more complicated version of this.