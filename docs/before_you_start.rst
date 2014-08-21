Before you start
================

.. _before_you_start:

The project description, licensing, source code and how to compile
are available `here <https://github.com/steinwurf/kodo-ns3-examples>`_.
Please follow the instructions shown there to get the project running.
**Once you have built the project**, you can follow this tutorial. If for some
reason your project does not build, please feel free to contact us through
our `developers mailing list <http://groups.google.com/group/steinwurf-dev>`_.

Examples overview
-----------------

Currently the repository ``kodo-ns3-examples`` contains 3 basic examples
in its main path regarding how to use the library with ns-3, namely:

* ``wifi_broadcast``: This example consists on broadcasting packets
  with RLNC on a single transmitter receiver pair with an IEEE 802.11b WiFi
  channel.
* ``wired_broadcast``: This example consists on broadcasting packets
  with RLNC from a transmitter to 2 receivers with the same erasure channel.
* ``encoder_recoder_decoder``: This example shows the gain of RLNC
  with recoding in a 2-hop line network consisting of an encoder, recoder and
  decoder with different erasure rates. Recoding can be set on or off and
  erasures rate modified by command line parsing.

Examples builds
---------------

You can check the build status of the repository master branch in our buildbot
page `here <http://buildbot.steinwurf.dk/stats?projects=kodo-ns3-examples>`_.

Platform and compiler support
---------------------------------

Currently we support the examples in 64-bit Linux (Debian Jessie) and Mac
(OSX Mavericks 10.9) with Python 2.7. For the compilers case, we run the builds
for GCC 4.8 and Clang 3.4. If you want to check a build for a particular
platform - compiler combination not listed here, please let us know through our
developers mailing list.
