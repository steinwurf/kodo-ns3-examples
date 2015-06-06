Introduction
------------
This repository contains examples that show you how to build a standalone
ns-3 simulation using the Kodo erasure coding library 
(http://steinwurf.com/kodo/).

A valid Kodo license is required if you wish to use this project.
Please request a license by **filling out the license request** form_.

Kodo is available under a research- and education-friendly license,
you can see the details here_.

.. _form: http://steinwurf.com/license/
.. _here: http://steinwurf.com/research-license/

ns-3 (http://nsnam.org) is a discrete-event network simulator, targeted
primarily for research and educational use. ns-3 is licensed under the GNU
GPLv2 license.

.. image:: http://buildbot.steinwurf.dk/svgstatus?project=kodo-ns3-examples
    :target: http://buildbot.steinwurf.dk/stats?projects=kodo-ns3-examples
    :alt: Buildbot status

Using ns-3 as a Library
-----------------------
In the example provided in this repository we use ns-3 as a library, i.e.
we build ns-3 separately and then simply link against it to build our
simulation. This has several advantages over developing the 
simulation by directly modifying the ns-3 source code or its examples
(of course, this list is purely subjective and you are free to disagree).

* It becomes easier to upgrade to the next version of ns-3 as it becomes
  available.
* If you want to distribute your changes, it can be done easily without
  distributing the entire ns-3 simulator.
* You can freely choose whatever build system you prefer. Note that we
  also use the same build system as ns-3, namely Waf, but we use
  a different version of waf!

Getting Started
---------------
As a first step you need ns-3 installed on your development machine.
You may find lots of information about this on the ns-3 webpage.

http://www.nsnam.org/wiki/index.php/Installation.

On Ubuntu/Debian, you need to install the following packages::

  sudo apt-get install g++ python mercurial git-core

Below we have recorded the steps needed to get ns-3 up and running.

Installing ns-3
---------------
First clone the ns-3 repository (we start from the home folder
here as an example, but you can change this)::

  cd ~
  hg clone http://code.nsnam.org/ns-3-dev/

This will download the ns-3 simulator to your computer into
the ``ns-3-dev`` folder. We will switch to the specific ns-3 release,
which is currently supported by the examples.

Check the current tagged version of ns-3::

  cd ns-3-dev
  hg tags

We will select the currently supported release::

  hg checkout ns-3.21

Configure the ns-3 project by running::

  python waf configure

This will output a whole bunch of information about the modules
enabled based on the availability of tools and libraries installed
on your development machine. We will only need the ``Real Time Simulator``,
this should be marked ``enabled``. Now we can build the
ns-3 simulator libraries::

  python waf build

The ns-3 libraries should be built and we will use them in our
simulations.

Building the Kodo ns-3 examples
-------------------------------
After building ns-3, you can build the example simulations provided in this
repository.

First you have to clone this repository. Note that you *should not* clone
the repository inside your ``ns-3-dev`` folder, because the examples will not 
work there. For example, this clone command will create the ``kodo-ns3-examples``
folder in your home folder::

  cd ~
  git clone git@github.com:steinwurf/kodo-ns3-examples.git

Configure and build the project::

  cd kodo-ns3-examples
  python waf configure --ns3-path=~/ns-3-dev

The ``waf configure`` command ensures that all dependencies are downloaded
(by default, waf will create a folder called  ``bundle_dependencies`` to
store these libraries). The ``--ns3-path`` specifies the folder where
you made the ns-3 checkout and built the ns-3 libraries.

Now you can build the examples by running::
  
  python waf build

Currently we have the following examples:

* ``wifi_broadcast``: This example demonstrates broadcasting packets with RLNC
  to N receivers over an IEEE 802.11b WiFi channel.
* ``wired_broadcast``: This example demonstrates broadcasting packets with RLNC
  from a transmitter to N receivers with the same erasure channel.
* ``encoder_recoder_decoder``: This example shows the gain of RLNC with recoding
  in a 2-hop line network consisting of an encoder, N recoders and a decoder with
  different erasure rates. Recoding can be turned on or off and erasure rates
  can be modified by command line options.

You can see more documentation of each example in the ``main.cc`` file comments
regarding what each example does. There you can also check how to control and
set up the simulation parameters like packet, field and generation sizes
among others.

The build command will generate binaries in the ``./build/linux/src`` folder, 
one for each example. Probably the build will show some warnings, but if it is
successful, you will be able to run the examples.

Try running the ``wifi_broadcast`` example by typing: ::

  ./build/linux/src/wifi_broadcast/wifi_broadcast

You should see how the decoding matrix changes with each combination sent. 
You will see if a received packet is linearly dependent or not. You
will also see when the decoding process is completed and how many transmissions
were required.

Tutorial
--------
Our `Kodo-ns3 tutorial <http://kodo-ns3-examples.readthedocs.org/en/latest/>`_ 
provides a more comprehensive description of these examples.
For each example, we verify known results to validate our model and provide 
parameters to the user for modifying them.

Comments, Feedback & Bugs
-------------------------
All comments, questions and feedback regarding the examples should be
posted to our developer mailing list (hosted at Google Groups):

* http://groups.google.com/group/steinwurf-dev

Any bugs and patches should be posted to the github issue tracker:

* https://github.com/steinwurf/kodo/issues

If you make new examples or use the examples provided here for your
research please let us know - we would be happy to add links to your
work or potentially include it as new examples.
