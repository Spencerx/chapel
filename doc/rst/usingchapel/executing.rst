.. _readme-executing:

=========================
Executing Chapel Programs
=========================

Once you have compiled a Chapel program using the chpl compiler, you
can execute it from the command-line like any other program.  This
document discusses command line options, environment variables, and
other topics having to do with running programs written in Chapel.

.. contents::


------------
Getting Help
------------

Using the ``-h`` or ``--help`` flags will print out help for the
executable.  For example:

  .. code-block:: sh

    ./myprogram --help

This flag lists all of the standard flags that can be used with a
Chapel program, as well as a list of the configuration variables
defined by the program and their types.  If the configuration variable
has been set on the command line, its value is also shown.


-------------------------------
Setting Configuration Variables
-------------------------------

~~~~~~~~~~~~
Command Line
~~~~~~~~~~~~

:ref:`Configuration constants and variables <ug-configs>` defined in a Chapel
program can have their default values overridden on the command line using the
``-s`` or ``--`` flags.  Either flag takes the name of the configuration
variable followed by an equals character (``=``) and the value to assign to it.
This value must be a legal Chapel literal for the type of the variable.
(Exception: for a string literal, the surrounding quotes are implicit.)
In our current implementation, no extra spaces may appear between
these elements.  Thus, the general form is:

  .. code-block:: sh

    ./myprogram --<cfgVar>=<val>

or:

  .. code-block:: sh

    ./myprogram -s<cfgVar>=<val>

As an example, compile the hello2-module.chpl example which prints a
user-definable message:

  .. code-block:: sh

    chpl -o hello2-module $CHPL_HOME/examples/hello2-module.chpl

This program defines a configuration constant, ``message``, indicating
the string to print to the console, set to ``"Hello, world!"`` by
default.  To override the default, you can use:

  .. code-block:: sh

    ./hello2-module --message='Hello, Chapel users!'

or:

  .. code-block:: sh

    ./hello2-module -smessage='Hello, Chapel users!'

The default value of a configuration constant or variable can also be
overridden at compilation time with a ``-s`` option. (The surrounding
quotes must be provided for a string literal.) For example:

  .. code-block:: sh

    cd $CHPL_HOME/examples
    chpl -smessage='"Hello from the Chapel compiler"' hello2-module.chpl

or:

  .. code-block:: sh

    chpl -s message="'Hello from the Chapel compiler'" hello2-module.chpl

The compiler-established default can still be overridden when
executing the program, as shown above.

Command-line overrides of configs may also use fully qualified names.
Thus, each of the above examples could have referred to ``message`` as
``Hello.message`` instead.  This is useful for disambiguating when
multiple modules declare configs with the same name.  In addition,
``private config`` declarations `must` be set using fully-qualified
names.

~~~~~~~~~~~~~~~~~~
Configuration File
~~~~~~~~~~~~~~~~~~

Configuration values can also be passed to a Chapel program through a
configuration file, specified by the execution time ``-f`` option.
Configuration files can contain a whitespace- or newline-delimited list of
keys and values separated by an assignment operator ``=``. Comments begin
with the ``#`` character. The examples below demonstrate this format.

Consider the following program:

   .. code-block:: chapel

       // program.chpl
       config const msg: string,
                    val1: real,
                    val2: real;


       proc main() {
         writeln(msg);
         writeln(val1);
         writeln(val2);
       }

The above program can have its configuration variables defined by this
configuration file:

    .. code-block:: python

        # program.input

        msg="hello world"
        val1=1.61803
        val2=3.14159

Configuration files can contain a whitespace- or newline-separated list of
configuration assignments and comments are supported with the ``#`` character.
The configuration file above can also be written like this:

    .. code-block:: python

        # program.input

        val1=1.61803 val2=3.14159
        msg="hello world" # This is a comment


The ``program.input`` is passed during execution with the ``-f`` flag:

    .. code-block:: sh

        # config variables are populated by program.input values
        ./program -fprogram.input


.. warning::
    Assignments cannot contain whitespaces outside of quotes, so the following
    configuration file would result in an error:

        .. code-block:: python

            # bad.input

            # The additional whitespace will result in an error
            val1 = 1.161803


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Non-Configuration Variable Arguments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Chapel programs can also accept C-like command line arguments to their
``main()`` procedure in addition to the aforementioned configuration
variables. See :ref:`readme-main` for more
information.


---------------------
Multi-locale Programs
---------------------

For multi-locale Chapel programs, the number of locales is specified on the
executable's command-line.  This can be set either using the ``-nl`` flag
(e.g., ``-nl 4``), or by assigning to the built-in ``numLocales``
configuration constant (e.g., ``-snumLocales=4`` or ``--numLocales=4``).
See :ref:`readme-multilocale` for more information about multiple locales,
and the methods for specifying the number of locales and nodes.

Multi-locale programs require a communication layer (i.e., you cannot run
multi-locale programs if ``CHPL_COMM=none``). See :ref:`readme-multilocale`
for information about choosing the proper communication layer for your
platform.

Multi-locale programs typically use a launcher executable (e.g., ``hello``)
that does some initial command-line checking before using a system launcher
(e.g., ``slurm``) to launch the real program. The real program is named
``<original_binary_name>_real`` (e.g., ``hello_real``). See
:ref:`readme-launcher` for more information about launchers and
the launcher executable.

--------------------------------------
Controlling Degree of Data Parallelism
--------------------------------------

Data parallel operations over ranges, default domains, and default
arrays permit the number of tasks used to implement the data
parallelism to be specified using the following built-in configuration
constants:

  ``dataParTasksPerLocale``
    Number of Chapel tasks to use to execute forall loops (default:
    number of physical CPUs on the node, which may be reduced by
    ``CHPL_RT_NUM_THREADS_PER_LOCALE``.  When ``CHPL_TASKS=qthreads``,
    ``CHPL_RT_NUM_THREADS_PER_LOCALE`` can also increase this, up to the
    number of logical CPUs).

  ``dataParIgnoreRunningTasks``
    If ``true``, always use ``dataParTasksPerLocale`` tasks to execute
    forall loops.  If ``false``, reduce the number of tasks used by the
    number of tasks already running (default: ``false``).

  ``dataParMinGranularity``
    The number of tasks used to execute forall loops should be reduced
    such that the number of iterations per task is never less than the
    specified value (default: ``1``).

Most Chapel standard distributions also use identically named
constructor arguments to control the degree of data parallelism within
each locale when iterating over its domains and arrays.  The default
values for these arguments are set to the corresponding global
configuration constants.


-------------------------------------------------
Environment Variable Control over Chapel Behavior
-------------------------------------------------

Chapel uses environment variables to control the number of threads used
at execution time and the call stack size, among other things.  In many
cases third-party packages used by Chapel define their own environment
variables to provide the same or similar control.  When this is the
case, the Chapel environment variable has precedence over the
third-party package environment variable, which in turn has precedence
over the Chapel default.

As an example, with ``CHPL_TASKS=qthreads``, Chapel and Qthreads both
have environment variables that can be used to set the task stack size.
In this case the Chapel ``CHPL_RT_CALL_STACK_SIZE`` environment variable
will override the Qthreads ``QT_STACK_SIZE`` environment variable if
both are set.  However, if only ``QT_STACK_SIZE`` is set it will
override the Chapel default call stack size.

The Chapel environment variables that control execution time behavior
are as follows:

  ``CHPL_RT_CALL_STACK_SIZE``
    size of the call stack for a task

  ``CHPL_RT_MAX_HEAP_SIZE``
    per-locale size of the heap used for dynamic allocation in
    multilocale programs

  ``CHPL_RT_NUM_THREADS_PER_LOCALE``
    number of threads used to execute tasks

  ``CHPL_RT_USE_PU_KIND``
    which kind of processing units to use on CPUs with hybrid
    processing units. Values are "performance", "efficiency", or "all".

There is a bit more information on ``CHPL_RT_CALL_STACK_SIZE``,
``CHPL_RT_NUM_THREADS_PER_LOCALE``, and ``CHPL_RT_USE_PU_KIND`` below,
and more detailed discussions
in :ref:`readme-tasks` and :ref:`readme-cray`.


-------------------------------
Controlling the Call Stack Size
-------------------------------

The main Chapel program requires space for its call stack, as does any
task created by it.  This stack space has a fixed size.  It is created
automatically when the program or task starts executing, and remains in
existence until it completes.  The default call stack size is ~8 MiB on
Linux-based systems, since this is a common value for the process stack
limit on such systems.  On Cygwin systems the default call stack size is
~2 MiB. Note that up to 4 system pages of each stack may be reserved for
use by the tasking layer. Up to 2 pages for runtime data structures and
up to 2 additional pages if guard pages (--stack-checks) are enabled.

The default call stack size may not be appropriate in all cases.  For
programs in which some tasks have large stack frames or deep call trees
it may be too small, leading to stack overflow.  For programs which use
tasks only for data parallelism it may be unnecessarily large.  Stacks
that are unnecessarily large are typically only a problem for programs
in which many tasks (thus their stacks) exist at once, when using a comm
layer that has to pre-register memory.  For the particular case of using
the native runtime communication layers on Cray XC and HPE Cray EX systems,
further discussion about this can be found in :ref:`readme-cray`.

The following environment variable can be used to change the task call
stack size.

  ``CHPL_RT_CALL_STACK_SIZE``
    Size of the call stack for a task.  A plain numeric value indicates
    bytes.  A suffix can be appended to indicate larger units:

     | ``k``, ``K``: KiB (2**10 bytes)
     | ``m``, ``M``: MiB (2**20 bytes)
     | ``g``, ``G``: GiB (2**30 bytes)


.. _num-threads-per-locale:

---------------------------------
Controlling the Number of Threads
---------------------------------

The following environment variable can be used to change the number of
system threads used by a program.

  ``CHPL_RT_NUM_THREADS_PER_LOCALE``
    Controls the number of threads used on each locale when running the
    program.

See :ref:`readme-tasks` for more information on the role of
this variable in creating threads and executing tasks for the various
tasking layers.


----------------------------------------
Controlling the Kind of Processing Units
----------------------------------------

Some CPUs, such as Intel's "Alder Lake" family of processors, have
hybrid processing units, some of which are "performance" and others
of which are "efficiency". The following environment variable can be used
to select the kind of processing units used by a program.

  ``CHPL_RT_USE_PU_KIND``
    Specifies which kind of processing units to use on CPUs with hybrid
    processing units. Values are "performance", "efficiency", or "all".

By default the Chapel runtime will only use "performance" processing units.
Note that if set to "all" the runtime will run tasks on all available
cores/processing units indiscriminately ; it will not take the difference in
performance into account when assigning tasks to processing units.

This environment variable has no effect if the processor does not have hybrid
processing units.

-----------------------------------------
Controlling the Amount of Non-User Output
-----------------------------------------

The compiler-generated executable supports *verbose* and *quiet* modes
that control the amount of Chapel-generated information printed by the
executable.

    -v, --verbose  Print more information. For example, print the
                   launcher commands used to start the program (if any)
                   and print a message from each locale when the program
                   starts executing there.
    -q, --quiet    Print less information. For example, suppress run-time
                   warnings that are printed by default.


.. _oversubscribed-execution:

----------------
Oversubscription
----------------

In multi-locale Chapel executions programs can run "oversubscribed",
with more than one Chapel locale per system compute node.  Both the
``gasnet`` and ``ofi`` communication layers support this.  However,
oversubscription can cause serious performance degradation due to
resource contention, when multiple Chapel locales (program instances)
all proceed as if the resources of the entire compute node are theirs
to use.  As a result, on a single system node, a program will almost
always run faster with just a single locale than it will with multiple
locales.  Nevertheless, sometimes oversubscription is useful, such as
for testing multilocale Chapel functionality when multiple system
nodes are not actually available.

As a partial workaround for the resource contention problem, setting
the following environment variable often improves performance when
running oversubscribed:

    .. code-block:: sh

        export CHPL_RT_OVERSUBSCRIBED=yes

This causes various software components, from launchers to the
runtime, to be more considerate in how they use node resources.


----------------
Stack Traces
----------------

When compiled with unwind support (``CHPL_UNWIND=bundled`` or
``CHPL_UNWIND=system``), Chapel can print a stacktrace when it halts due to an
error. For example, when compiling and running the following program:

   .. code-block:: chapel

       // outofbounds.chpl
       var A: [1..3] real;
       A[5] = 1.0;

   .. code-block:: sh

        ./outofbounds

   .. code-block:: sh

        outofbounds.chpl:2: error: halt reached - array index out of bounds
        note: index was 5 but array bounds are 1..3
        Stacktrace

        halt() at $CHPL_HOME/modules/standard/Errors.chpl:741
        checkAccess() at $CHPL_HOME/modules/internal/ChapelArray.chpl:2675
        chpl__init_outofbounds() at outofbounds.chpl:1

The stacktrace will be printed. This behavior can also be disabled at runtime
with the environment variable

    .. code-block:: sh

        export CHPL_RT_UNWIND=no

Note that compiling programs with ``--fast`` effectively forces
``CHPL_RT_UNWIND=no`` so that no stacktrace will be printed even if the Chapel
compiler was built with unwind support. And in the above example, ``--fast``
also omits the bounds check.


----------------
Launcher Support
----------------

For multilocale execution (see :ref:`readme-multilocale`),
Chapel programs are executed indirectly by a launcher.  This section
covers command line options that assist launchers in doing their job.
These options are not supported for general use.  We document them here
so that their presence in, say, the output produced by ``-v`` or ``--dry-run``
can be understood.

At present there is only one launcher support option:

  -E <envVar=val>  set the given environment variable *envVar* to *val*.
