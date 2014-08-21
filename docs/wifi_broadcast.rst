Broadcast RLNC with a WiFi channel
==================================

.. _wifi_broadcast:

The topology shown in Fig. **XX** describes a transmitter sending coded packets
with RLNC from a generation size :math: `g` and field size :math: `q` in a
broadcast fashion through a 802.11b channel. For the purpose of our example
we will start with :math: `g = 5` and :math:i `q = 2` (i.e. the binary field) and
we will check the completion time in terms of transmissions through the WiFi
channel under different situations.

What to simulate?
-----------------

We will consider the following guidelines for our simulation:

* Behaviour: The sender keeps transmitting the generation until the
  receiver has :math:`g` linearly independent (l.i.) coded packets
  (combinations). Packets might or might not be loss due to channel
  impairments.
* Inputs: As main parameters regarding RLNC, we choose the generation
  and field size. A parameter regarding channel control should be included
* Outputs: A counter to indicate how much transmission did the process
  required and some prints to indicate when decoding is completed.
* Scenarios: We will variate the generation and field size to verify
  theoretical expected values regarding the amount of transmissions to
  decode. Also, the number of transmission should somehow change as we
  vary the channel.

Main program description
------------------------

After the project has been properly configured and built, you should have
a folder named ``~/dev/kodo-ns3-examples/wifi_broadcast/`` where ``~/dev/`` is
the folder where you cloned the project repository. If you check the
``wifi_broadcast`` folder, you will see the ``main.cpp`` file with contains
the source code of this simulation. You can open it with your preferred editor
to review the source code. We will briefly review some of its parts.

Overview comments and includes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. code:: c++

   // ns-3 document style descriptor for e-macs

   // ns-3 license disclaimer

   // Example description

   // ns-3 header includes
   #include <ns3/core-module.h>
   #include <ns3/network-module.h>
   #include <ns3/mobility-module.h>
   #include <ns3/config-store-module.h>
   #include <ns3/wifi-module.h>
   #include <ns3/internet-module.h>

   // Kodo includes
   #include <kodo/rlnc/full_rlnc_codes.hpp>
   #include <kodo/trace.hpp>

   // Simulation includes
   #include <iostream>
   #include <fstream>
   #include <vector>
   #include <string>
   #include <ctime>

As with any source code, the overview comments provide a reference to the users
regarding project licensing and a brief introduction to what we are simulating.
The includes are ordered from the most general to particular functionalities
within ns-3 and Kodo. From ns-3, the necessary modules are:

* Core module: For simulation event handling. This module provide a set of
  class-based APIs that control the simulation behaviour. It is essential for
  every ns-3 simulation.
* Network module: For creation and management of simulated devices. ns-3 has
  mainly two building blocks which represent a transmission/reception
  interface on physical equipment, namely **nodes** ("equipment") from a
  networking perspective and **net devices** ("interface cards") which actually
  deal with the physical layer. A node may have various net devices, but a net
  device cannot be shared by various nodes. We will also use the ``Packet``
  and ``ErrorModel`` classes from this module to represent other simulation
  objects.
* Mobility module: For providing a description of how the nodes move in an
  environment. We will use briefly this module for a representation of our
  physical channel.
* Config-store module: A specialized ns-3 database for internal attributes and
  default values for the different APIs.
* WiFi module: A PHY and MAC layer models for WiFi. Necessary for our medium
  access control.
* Internet module: For handling the IPv4 protocol at the network layer.

From Kodo, the header ``full_rlnc_codes`` contains the description of the
encoder and decoder objects that we use. ``trace`` is an internal class to
provide a visual description of internal encoder and decoder processing. Other
includes are particular to this implementation.


