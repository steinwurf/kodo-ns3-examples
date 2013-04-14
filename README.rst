Introduction
------------
This repository contains examples that show how you can build an ns-3
using the Kodo erasure coding library.

ns-3 is a discrete-event network simulator, targeted primarily for
research and educational use. ns-3 is licensed under the GNU GPLv2 license.

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
