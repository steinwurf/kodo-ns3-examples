The kodo-ns3-examples repository demonstrates how to use the Kodo erasure
coding library (http://steinwurf.com/kodo/) in various ns-3 examples.

ns-3 (http://nsnam.org) is a discrete-event network simulator, targeted
primarily for research and educational use. ns-3 is licensed under the GNU
GPLv2 license.

The kodo-ns3-examples repository: https://github.com/steinwurf/kodo-ns3-examples

.. image:: http://buildbot.steinwurf.dk/svgstatus?project=kodo-ns3-examples
    :target: http://buildbot.steinwurf.dk/stats?projects=kodo-ns3-examples
    :alt: Buildbot status

If you have any questions or suggestions about this library, please contact
us at our developer mailing list (hosted at Google Groups):

* http://groups.google.com/group/steinwurf-dev

**Important:** Before asking any questions, please try to precisely follow the
instructions here and read the **guidelines** of our mailing list!

If you have a technical issue with ns-3 itself, then please ask around on
the `ns-3 mailing list <https://groups.google.com/d/forum/ns-3-users>`_.

License
-------

A valid Kodo license is required if you wish to use this project.
Please request a license by **filling out the license request** form_.

Kodo is available under a research- and education-friendly license,
you can see the details here_.

If you try to configure without a valid license, then you will get an error!

.. _form: http://steinwurf.com/license.html
.. _here: http://steinwurf.com/research/evaluation-license.html

Documentation
-------------

Please read our general documentation here to get started:
http://docs.steinwurf.com

The kodo-ns3-examples tutorial is located here:
http://docs.steinwurf.com/kodo-ns3-examples/master/index.html

The tutorial provides a comprehensive description of our examples, and it
is designed to be a starting point for ns-3 developers that intend to use
the Kodo library for a specific application of Random Linear Network Coding
(RLNC) within the simulator.

Using Kodo in an ns-3 example
-----------------------------
The examples in this repository will be installed to the ``examples/kodo``
subfolder of your ns-3 folder. We will also build the Kodo static libraries
which will be installed in the same folder together with the required header
files. The examples will be linked with the required static libraries.

Getting Started
---------------
As a first step, you need ns-3 installed on your development machine.
You may find lots of information about this on the ns-3 webpage:

http://www.nsnam.org/wiki/index.php/Installation

On Ubuntu/Debian, you need to install the following packages::

  sudo apt-get install g++ python mercurial git-core

In the following, we will clone ns-3 to the ``~/ns-3-dev`` folder and we
will clone the kodo-ns3-examples to the ``~/kodo-ns3-examples`` folder.
You may use different folders, but the two folders **must be separate**,
i.e. one cannot be the subfolder of the other.

Installing ns-3
---------------

First clone the ns-3 repository (we start from the home folder,
so it will be cloned to ``~/ns-3-dev``)::

  cd ~
  hg clone http://code.nsnam.org/ns-3-dev/

This command will download the ns-3 simulator to your computer into
the ``ns-3-dev`` folder (this may take a few minutes).

Go to this freshly cloned folder::

  cd ns-3-dev

Our aim is to make the examples compatible with the latest ns-3 revision.
If you experience any issues with the latest revision, then you can switch
to the latest supported revision (this step is **optional**)::

  hg checkout 12799

Our build system automatically tests the examples with the latest supported
revision of ns-3. This revision is specified in our buildbot.py_ script.

.. _buildbot.py: https://github.com/steinwurf/kodo-ns3-examples/blob/master/buildbot.py#L73

Configure the ns-3 project (it is important to also enable the examples)::

  python waf configure --enable-examples

This will output a whole bunch of information about the enabled modules
based on the availability of tools and libraries installed on your computer.

Now we build the ns-3 libraries and examples::

  python waf build

Installing the Kodo examples to ns-3
------------------------------------
After building ns-3, you can switch to the kodo-ns3-examples repository.
We will clone this repository to the ``~/kodo-ns3-examples`` folder::

  cd ~
  git clone git@github.com:steinwurf/kodo-ns3-examples.git

Configure this project::

  cd kodo-ns3-examples
  python waf configure

The ``waf configure`` command ensures that all dependencies are downloaded
(by default, waf will create a folder called ``bundle_dependencies`` to
store these libraries).

You must have **a valid Steinwurf license** to download the ``fifi`` and
``kodo`` dependencies, otherwise you will get a git error when you execute
the configure command!

Now we build the kodo-rlnc static library and we install the examples and all
the required files to the ``~/ns-3-dev/examples/kodo`` folder::

  python waf build install --ns3_path="~/ns-3-dev"

The ``--ns3_path`` option is used to specify your ns-3 folder (you can change
this if your ns-3 is located elsewhere).

Building the Kodo examples in ns-3
----------------------------------

After the install step, you can switch back to your ns-3 folder::

  cd ~/ns-3-dev

You can follow the normal ns-3 workflow to build our examples. The ns-3 waf
will automatically find the new examples in ``~/ns-3-dev/examples/kodo``::

  python waf build

We have the following examples:

* ``kodo-wired-broadcast``: This example demonstrates broadcasting packets
  with RLNC from a transmitter to N receivers with the same erasure channel.

* ``kodo-wifi-broadcast``: This example demonstrates broadcasting packets
  with RLNC to N receivers over an IEEE 802.11b WiFi channel.

* ``kodo-recoders``: This example shows the gain of RLNC with recoding
  in a 2-hop line network consisting of an encoder, N recoders and a decoder
  with different erasure rates. Recoding can be turned on or off and the
  erasure rates can be modified by command-line options.

You can find more details about each example in their respective source files.
There you can also check how to change the simulation parameters like
the packet-, field- and generation sizes.

You can run the examples with the usual ns-3 run commands::

  python waf --run kodo-wired-broadcast
  python waf --run kodo-wifi-broadcast
  python waf --run kodo-recoders

Most of the examples will print out how the decoding matrix changes with
each combination packet. You will see if a received packet is linearly
dependent or not. You will also see when the decoding process is completed
and how many transmissions were required.

Adding your own simulation
--------------------------
At this point, you might want to add your own simulation that uses kodo.
It is recommended to create a new program by copying one of the kodo examples.
The examples are installed in ``~/ns-3-dev/examples/kodo``, so we will go to
that folder and make a copy ``kodo-wifi-broadcast.cc`` to create a new
simulation called ``my-simulation.cc`` (you can choose any name here)::

  cd ~/ns-3-dev/examples/kodo
  cp kodo-wifi-broadcast.cc my-simulation.cc

To build an executable from the ``my-simulation.cc`` source file, we have to
define a new program in ``~/ns-3-dev/examples/kodo/wscript``.
Open this file in your text editor, and add the following lines at the end
(be careful with the indentation since this is a Python script)::

  obj = bld.create_ns3_program('my-simulation',
                               ['core', 'applications', 'point-to-point',
                                'point-to-point-layout', 'internet', 'wifi'])
  obj.source = 'my-simulation.cc'
  set_properties(obj)

After this change, the ns-3 waf will detect the new example and you will be
able to run it from the ``~/ns-3-dev`` folder as usual::

  cd ~/ns-3-dev
  python waf --run my-simulation

Now you can expand your custom simulation as you like. If you use additional
ns-3 modules, then you need to add them in the wscript (most likely, you will
get a build error if you are missing a module).

If your simulation has multiple source files (*.cc files), then you can add
these in the wscript like this::

  obj.source = ['my-simulation.cc', 'source2.cc', 'source3.cc']

**Warning:** If you install the kodo ns-3 examples again with this command::

  cd ~/kodo-ns3-examples
  python waf build install --ns3_path="~/ns-3-dev"

then the example source files and the wscript will be overwritten in
the ``~/ns-3-dev/examples/kodo`` folder, so it is recommended to create a
backup if you modified any of these files.

You can also create a separate folder for your custom simulation to avoid this
problem. For example, you can create the ``~/ns-3-dev/examples/my-simulation``
folder and copy the ``include`` and ``lib`` folders from
``~/ns-3-dev/examples/kodo``.
