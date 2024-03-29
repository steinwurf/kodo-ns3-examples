Kodo NS3 Examples
=================

The kodo-ns3-examples repository demonstrates how to use the Kodo erasure
coding library (http://kodo.steinwurf.com) in various ns-3 examples.

ns-3 (http://nsnam.org) is a discrete-event network simulator, targeted
primarily for research and educational use. ns-3 is licensed under the GNU
GPLv2 license.

The kodo-ns3-examples repository: https://github.com/steinwurf/kodo-ns3-examples

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

If you try to configure without a valid license, then you will get an error!

.. _form: https://www.steinwurf.com/research-license-request

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

  sudo apt-get install g++ python3 git

In the following, we will clone ns-3 to the ``~/ns-3-dev`` folder and we
will clone the kodo-ns3-examples to the ``~/kodo-ns3-examples`` folder.
You may use different folders, but the two folders **must be separate**,
i.e. one cannot be the subfolder of the other.

Installing ns-3
---------------

First clone the ns-3 repository (we start from the home folder,
so it will be cloned to ``~/ns-3-dev``)::

  cd ~
  git clone https://gitlab.com/nsnam/ns-3-dev.git

This command will download the ns-3 simulator to your computer into
the ``ns-3-dev`` folder (this may take a few minutes).

Go to this freshly cloned folder::

  cd ns-3-dev

Our aim is to make the examples compatible with the latest ns-3 revision.
If you experience any issues with the latest revision, then you can switch
to the latest stable release (this step is **optional**)::

  git checkout ns-3.35

Configure the ns-3 project (it is important to also enable the examples)::

  python3 waf configure --enable-examples

This will output a whole bunch of information about the enabled modules
based on the availability of tools and libraries installed on your computer.

Now we build the ns-3 libraries and examples::

  python3 waf build

Installing the Kodo examples to ns-3
------------------------------------
After building ns-3, you can switch to the kodo-ns3-examples repository.
We will clone this repository to the ``~/kodo-ns3-examples`` folder::

  cd ~
  git clone git@github.com:steinwurf/kodo-ns3-examples.git

Configure this project::

  cd kodo-ns3-examples
  python3 waf configure

The ``waf configure`` command ensures that all dependencies are downloaded
(by default, waf will create a folder called ``bundle_dependencies`` to
store these libraries).

Now we build the kodo-rlnc static library and we install the examples and all
the required files to the ``~/ns-3-dev/examples/kodo`` folder::

  python3 waf build install --destdir ~/ns-3-dev/examples/kodo

The ``--destdir`` option is used to specify the target folder (you can change
the ``kodo`` subfolder name to something else if you like).

Building the Kodo examples in ns-3
----------------------------------

After the install step, you can switch back to your ns-3 folder::

  cd ~/ns-3-dev

You can follow the normal ns-3 workflow to build our examples. The ns-3 waf
will automatically find the new examples in ``~/ns-3-dev/examples/kodo``::

  python3 waf build

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

  python3 waf --run kodo-wired-broadcast
  python3 waf --run kodo-wifi-broadcast
  python3 waf --run kodo-recoders

Most of the examples will print out how the decoding matrix changes with
each combination packet. You will see if a received packet is linearly
dependent or not. You will also see when the decoding process is completed
and how many transmissions were required.

Adding your own simulation
--------------------------

At this point, you might want to add your own simulation that uses kodo.

It is recommended to create a separate folder (e.g.
``~/ns-3-dev/examples/my-simulation``) for your custom simulation and copy all
the necessary files from ``~/ns-3-dev/examples/kodo`` (most importantly the
``include`` and ``lib`` folders).

If you copy the wscript file, then please delete or comment out the parts where
we call ``bld.create_ns3_program``. We cannot have multiple programs with
the same name (e.g. ``kodo-recoders``).

When you create a new program, you can start by copying one of the kodo examples.
If you have a source file called ``my-simulation.cc``, then you can
define a new program in ``~/ns-3-dev/examples/my-simulation/wscript``.
like this::

  obj = bld.create_ns3_program('my-simulation',
                               ['core', 'applications', 'point-to-point',
                                'point-to-point-layout', 'internet', 'wifi'])
  obj.source = 'my-simulation.cc'
  set_properties(obj)

After this change, the ns-3 waf will detect the new example and you will be
able to run it from the ``~/ns-3-dev`` folder as usual::

  cd ~/ns-3-dev
  python3 waf --run my-simulation

Now you can expand your custom simulation as you like. If you use additional
ns-3 modules, then you need to add them in the wscript (most likely, you will
get a build error if you are missing a module).

If your simulation has multiple source files (.cc files), then you can add
these in the wscript like this::

  obj.source = ['my-simulation.cc', 'source2.cc', 'source3.cc']

**Warning:** If you install the kodo ns-3 examples again with this command::

  cd ~/kodo-ns3-examples
  python3 waf build install --destdir="~/ns-3-dev/examples/kodo"

then the example source files and the wscript will be overwritten in
the ``~/ns-3-dev/examples/kodo`` folder, so it is recommended to create a
backup if you modified any of these files.
