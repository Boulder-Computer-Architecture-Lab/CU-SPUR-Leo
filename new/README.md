# support status
currently, code compiles and works with ``gcc 13.1.1`` (and associated ``glibc``) for ``amd64``.

tested gem5 host architectures:
- ``aarch64``
- ``amd64``

the base binary without command-line options has been tested and confirmed to work on all gem5 builds.


# building / running tests
to build, run ``make`` in ``new/``. this also generates binaries for each individual test in ``btb/``, ``rsb/``, and ``pht/``, which can be used to verify the correctness of the specific attack and/or to test for compatibility with a given system.

manually setting ``CACHE_MISS`` in ``defines.h`` may improve sidechannel resolution. in my experience, ``80`` has worked for both gem5 and my native linux installation. 


# modifying benchmarks
this directory is structured as follows.
- root: utilities, main, global macros
  - ``main.c`` contains ``readMemoryBrace``, which tries a test a given number of times and extracts the result via a flush-reload sidechannel.
- pht, rsb, btb:
  - ``(name).h, (name).c``: contain the primary leaking elements of the attack. they are compiled into an object (``.o``) to be linked to the contents at root.
  - ``(name)_final.c``: contains an instance of the vulnerability, implemented to test the compiled object.

to add a new benchmark, you'll need to do the following:
- create a new folder with ``.h`` and ``.c`` files for the given vulnerability, as well as a makefile to compile the two to an object.
  - ensure that the new code is compatible with the ``readMemoryBrace`` function.
  - every pair contains a ``(name)_atk()`` function, which should be called during the actual test.
- add a new code for the test in ``defines.h``, a new ``argp`` entry in ``util.c``, and a new ``testTarget`` to ``main.c``. also add an entry to the switch segment in ``main()``.
  - make sure the ``testTarget`` is added to the same index in the ``tests`` array as the code defined in ``define.h``
- add the new object to the Makefile in root.

# notes on gem5 compatibility

your gem5 configuration may not support passing command line options to its workload by default.

there are two ways of working around this limitation:
- re-compilation with macros set
- modification of the gem5 configuration.

## re-compilation with macros

in ``defines.h``, under a section labelled ``command defaults``, you can adjust the default settings of program parameters.

of note here is the ``TESTS`` macro. by default, it should be set to 0, which enables all tests. to select specific tests, list the desired tests separated by ``|``, i.e.
```c
#define TESTS PHT_CODE | BTB_CODE
```

to select the PHT test and BTB test.

then, recompile, by running ``make`` in the ``/new`` directory.

## modification of gem5 configuration

these details are more relevant for the ``configs/learning_gem5/part1/two_level.py`` configuration, but are applicable to other cases.

to accept command line arguments:
- find some means of accepting the arguments to begin with
- include them in the ``process.cmd`` parametre of the ``Process()`` object which will be used as the workload

for example, for the ``two_level.py`` configuration, i added the following line:
```python
SimpleOpts.add_option("--bin-opts")
```
under the other ``SimpleOpts.add_option`` for ``"binary"``. 

then, i added the following below the line that sets ``process.cmd``:

```python
process.cmd = [args.binary]# this should already be there
if args.bin_opts != "": # this allows correct functionality if no options are provided
  process.cmd += args.bin_opts.split(" ")
```

this enables command-line arguments to be passed in like this:
```shell
build/x86/gem5.opt configs/learning_gem5/part1/two_level.py <other options> --bin-opts="-t -f" <binary>
```
(replace ``"-t -f"`` with whatever options you're using)

if you're not using the ``SimpleOpts`` module in ``configs/common``, then adding options may require a little more work with ``argparse`` in the configuration file. also see ``configs/example/se.py`` for a more detailed reference implementation.

