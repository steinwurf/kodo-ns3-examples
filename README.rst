Introduction
------------
This repository contains examples that show how you can build an ns-3
using the Kodo erasure coding library (github.com/steinwurf/kodo).
Which is freely available for educational and research purposes, you
may find the specific license in the library.

ns-3 is a discrete-event network simulator, targeted primarily for
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
using different approaches.

Clone ns-3 repository (Ubuntu)
------------------------------

First we need to make sure we have the tool required:
::
   sudo apt-get install gcc g++ python mercurial

Now clone the ns-3 repository:
::
  hg clone http://code.nsnam.org/ns-3-dev/

This will download the ns-3 simulator to your computer, into a
``ns-3-dev`` folder. We will switch  to the latest release. One
advantage or this approach is that is will be easy for us to
upgrade to the next version of ns-3 when it is release.

To see the current tagged version of ns-3 run:
::
  cd ns-3-dev
  hg tags

We will select the currently newest release:
::
  hg checkout ns-3.16

Configure the project by running:
::
  ./waf configure

This will output a whole bunch of information about the modules
enabled based on the availability of tools and libraries installed
on your development machine. We will only need the ``Real Time Simulator``,
this should be marked ``enabled``. Now we may proceed and build the
ns-3 simulator libraries:
::
  ./waf build

The ns-3 libraries should now be built and we may use them in our
simulations.

Comments, feedback & bugs
-------------------------
All comments, questions, and feedback regarding the examples can be
posted to our dev mailing list (hosted at google groups):

* xxx

Any bugs and patches should be posted to the github issue tracker:

* yyy



