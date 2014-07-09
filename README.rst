Introduction
------------
This repository contains examples that show how you can build an ns-3
using the Kodo erasure coding library (http://github.com/steinwurf/kodo).
To obtain a valid Kodo license you must fill out the license request form. For
educational and research purposes, you may find the specific license in the
library LICENSE.rst.

ns-3 (http://nsnam.org) is a discrete-event network simulator, targeted primarily for
research and educational use. ns-3 is licensed under the GNU GPLv2 license.

Using ns-3 as a library
-----------------------
In the example provided in this repository we use ns-3 as a library, i.e.
we build ns-3 separately and then simply link against it to build our
simulation. In contrast to developing the simulation by directly modifying
the ns-3 source code or putting it in the scratch folder. This has several
advantages (this is of course purely subjective and you are free to disagree).

* It becomes easier to upgrade to the next version of ns-3 as it becomes
  available.
* If you want to distribute your changes it can be done easily without
  distribution the entire ns-3 simulator.
* You can freely choose the whatever build system you prefer. Note, we
  also use the same build system as ns-3, namely Waf, because it rocks!

Getting Started
---------------
As a first step you need ns-3 installed on your development machine.
You may find lots of information about this on the ns-3 webpage.

http://www.nsnam.org/wiki/index.php/Installation.

Below we have recorded the steps needed to get ns-3 up and running
by cloning the repository.

Clone ns-3 repository (Ubuntu)
------------------------------
First we need to make sure we have the tool required: ::

  sudo apt-get install gcc g++ python mercurial

Now clone the ns-3 repository: ::

  hg clone http://code.nsnam.org/ns-3-dev/

This will download the ns-3 simulator to your computer, into a
``ns-3-dev`` folder. We will switch  to the latest release. One
advantage or this approach is that is will be easy for us to
upgrade to the next version of ns-3 when it is release.

To see the current tagged version of ns-3 run: ::

  cd ns-3-dev
  hg tags

We will select the currently newest release: ::

  hg checkout ns-3.20

Configure the project by running: ::

  ./waf configure

This will output a whole bunch of information about the modules
enabled based on the availability of tools and libraries installed
on your development machine. We will only need the ``Real Time Simulator``,
this should be marked ``enabled``. Now we may proceed and build the
ns-3 simulator libraries: ::

  ./waf build

The ns-3 libraries should now be built and we may use them in our
simulations.

Update to a new version
-----------------------
When a new version of ns-3 gets released you can get the new version easily by
running (in the ``ns-3-dev`` folder): ::

  hg pull

Then to see the tagged versions: ::

  hg tags

And as previously described to switch to version ``xx`` do a
(for example): ::

  hg checkout ns-3.xx

Now you have to go through the ``configure`` and ``build`` steps again,
described in the previous section.

Building an example simulation.
-------------------------------
After building ns-3 you can build one of the example simulations
provided by this repository. Here we will build the ``simple_udp_broadcast``
example.

Navigate to the ``simple_udp_broadcast`` folder: ::

  cd simple_udp_broadcast

Configure the project by running: ::

  ./waf configure --bundle-path=~/dev/bundle_dependencies --ns3-path=~/dev/ns-3-dev

The ``waf configure`` ensures that all tools needed by Kodo are
available and prepares to build Kodo.
Kodo relies on a number of auxiliary libraries (see kodo.readthedocs.org).
By specifying the ``--bundle-path`` option, this informs ``waf``
about where the downloaded libraries should be placed. You may
omit the ``--bundle-path`` option which in that case ``waf`` will create a
local directory in the Kodo folder called  ``bundle_dependencies`` and
store the libraries there. The ``--ns3-path`` specifies the folder where
you have made the ns-3 checkout and built the ns-3 libraries.

Now you should be able to build the simulation by running: ::

  ./waf build

Which will produce a binary in the ``build/linux/`` folder called
``simple_udp_broadcast``. Probably the build will throw some warnings, but if
it is successful, you will be able to continue the test.

Try running it by typing: ::

  ./build/linux/simple_udp_broadcast --verbose=1

In your terminal the ``--verbose`` option will make it print a lot
of info, just to see it works.

Comments, feedback & bugs
-------------------------
All comments, questions, and feedback regarding the examples can be
posted to our dev mailing list (hosted at google groups):

* http://groups.google.com/group/steinwurf-dev

Any bugs and patches should be posted to the github issue tracker:

* https://github.com/steinwurf/kodo/issues

If you make new examples or use the examples provided here for your
research please let us know - we would be happy to add links to your
work or potentially include it as new examples.
