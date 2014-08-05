Introduction
------------
This repository contains examples that show how you can build an ns-3
using the Kodo erasure coding library (http://github.com/steinwurf/kodo).

To obtain a valid Kodo license **you must fill out the license request** form_.
Kodo is available under a research and educational friendly license,
you can see the details here_.

.. _form: http://steinwurf.com/license/
.. _here: https://github.com/steinwurf/kodo/blob/master/LICENSE.rst

ns-3 (http://nsnam.org) is a discrete-event network simulator, targeted primarily
for research and educational use. ns-3 is licensed under the GNU GPLv2 license.

.. image:: http://buildbot.steinwurf.dk/svgstatus?project=kodo-ns3-examples
    :target: http://buildbot.steinwurf.dk/stats?projects=kodo-ns3-examples
    :alt: Buildbot status

Using ns-3 as a Library
-----------------------
In the example provided in this repository we use ns-3 as a library, i.e.
we build ns-3 separately and then simply link against it to build our
simulation. In contrast to developing the simulation by directly modifying
the ns-3 source code or putting it in the scratch folder. This has several
advantages (this is of course purely subjective and you are free to disagree).

* It becomes easier to upgrade to the next version of ns-3 as it becomes
  available.
* If you want to distribute your changes, it can be done easily without
  distributing the entire ns-3 simulator.
* You can freely choose whatever build system you prefer. Note that we
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

Configure the ns-3 project by running: ::

  ./waf configure

This will output a whole bunch of information about the modules
enabled based on the availability of tools and libraries installed
on your development machine. We will only need the ``Real Time Simulator``,
this should be marked ``enabled``. Now we may proceed and build the
ns-3 simulator libraries: ::

  ./waf build

The ns-3 libraries should now be built and we may use them in our
simulations.

Update to a New Version
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

Examples Description and Repository Building
--------------------------------------------
After building ns-3 you can build the example simulations provided by this
repository. Currently we have the following examples:

* ``wifi_broadcast``: This example consists on broadcasting packets with RLNC
  on a single transmitter receiver pair with an IEEE 802.11b WiFi channel.
* ``wired_broadcast``: This example consists on broadcasting packets with RLNC
  from a transmitter to 2 receivers with the same erasure channel.

You can see more documentation of each example in the ``main.cpp`` file comments
regarding what each example does. There you can also check how to control and set
up the simulation parameters like packet, field and generation sizes among others.

Configure the project by running: ::

  ./waf configure --bundle-path=~/dev/bundle_dependencies --ns3-path=~/dev/ns-3-dev

The ``waf configure`` ensures that all tools needed by Kodo are available and
prepares to build Kodo. Kodo relies on a number of auxiliary libraries
(see kodo.readthedocs.org). By specifying the ``--bundle-path`` option, this
informs ``waf`` about where the downloaded libraries should be placed. You may
omit the ``--bundle-path`` option which in that case ``waf`` will create a
local directory in the Kodo folder called  ``bundle_dependencies`` and
store the libraries there. The ``--ns3-path`` specifies the folder where
you have made the ns-3 checkout and built the ns-3 libraries.

Now you should be able to build the examples by running: ::

  ./waf build

This will produce different binaries in the ``./build/linux/`` folder, one per
example. Probably the build will throw some warnings, but if it is
successful, you will be able to continue the test.

Try running the ``wifi_broadcast`` example by typing: ::

  ./build/linux/wifi_broadcast/wifi_broadcast

You should see how the decoding matrix changes with each combination sent. Due
to the channel nature, here you will only see the linear dependence effect. You
will also see when the decoding process is completed and how many transmissions
where required.

Comments, Feedback & Bugs
-------------------------
All comments, questions, and feedback regarding the examples can be
posted to our dev mailing list (hosted at google groups):

* http://groups.google.com/group/steinwurf-dev

Any bugs and patches should be posted to the github issue tracker:

* https://github.com/steinwurf/kodo/issues

If you make new examples or use the examples provided here for your
research please let us know - we would be happy to add links to your
work or potentially include it as new examples.
