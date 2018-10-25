Before You Start
================

.. _before_you_start:

The project description, licensing, source code and how to compile
are available `here <https://github.com/steinwurf/kodo-ns3-examples>`_.
**Once you have compiled the project**, you can follow this tutorial.

Overview
--------

Currently the ``kodo-ns3-examples`` repository contains the following
basic examples:

* ``kodo-wifi-broadcast``: This example consists on broadcasting packets
  with RLNC from a transmitter to N receivers with an IEEE 802.11b WiFi
  channel.
* ``kodo-wired-broadcast``: This example consists on broadcasting packets
  with RLNC from a transmitter to N receivers with the same erasure channel.
* ``kodo-recoders``: This example shows the gain of RLNC
  with recoding in a 2-hop line network consisting of an encoder, N recoders and
  a decoder with different erasure rates. Recoding can be set on or off and
  erasure rates modified by command-line parsing.

What You Should Know Before the Tutorial
----------------------------------------

C++
^^^

In order for the tutorial to be easy to read, some basic knowledge of C++ is
recommended. If you are also a C++ beginner, you can refer to this
`tutorial <http://www.cplusplus.com/doc/tutorial/>`_ as a guide from the basics
to more advanced features of the language. Given that both ns-3 and Kodo are
highly object-oriented projects, we strongly recommend you to spend some
time on class-related topics, particularly object properties (polymorphism,
inheritance) and templates (generic classes or functions based on abstract
types). In the mentioned C++ tutorial you can find plenty of examples.

Kodo
^^^^

Kodo is a C++ library from `Steinwurf <http://www.steinwurf.com>`_ that
implements Random Linear Network Coding and its variants, but also other
codes like Reed-Solomon. You can read more about Kodo at:
http://docs.steinwurf.com


ns-3
^^^^

`ns-3 <http://www.nsnam.org/>`_ is a network simulator of the OSI layers
written in C++ for research and educational purposes under the GPLv2 license.
You can find documentation including a tutorial, a manual and a model
description here: http://www.nsnam.org/documentation
