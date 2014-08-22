Broadcast RLNC with a WiFi channel
==================================

.. _wifi_broadcast:

The topology shown in Fig. **XX** describes a transmitter sending coded packets
with RLNC from a generation size :math: `g` and field size :math: `q` in a
broadcast fashion through a 802.11b channel. For the purpose of our example
we will start with :math: `g = 5` and :math: `q = 2` (i.e. the binary field) and
we will check the completion time in terms of transmissions through the WiFi
channel under different situations.

What to simulate?
-----------------

We will consider the following guidelines for our simulation:

* Behaviour: The sender keeps transmitting the generation until the
  receiver has :math: `g` linearly independent (l.i.) coded packets
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

Program description
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

Required identifiers and types
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

We will be working within the ns-3 scope given that most of our objects are from
this library. This is typical across ns-3 code.

.. code:: c++

   using namespace ns3;

Also, for our enconder and decoder types, we need some ``typedefs`` to make
easy calls on them.

.. code:: c++

   typedef kodo::full_rlnc_encoder<fifi::binary,kodo::disable_trace> rlnc_encoder;
   typedef kodo::full_rlnc_decoder<fifi::binary,kodo::enable_trace> rlnc_decoder;

The RLNC encoder and decoder are template classes. The first input type is the
field size represented through an object (``struct`` in this case) from our
`Fifi  <https://github.com/steinwurf/fifi>`_ library. Fifi is a dependency for
Kodo where all the finite field arithmetics resides. Since we are interested in
:math: `q = 2` we choose ``fifi:binary``, however other field types from Fifi
might be chosen too according to your application. Current available filed sizes
are: :math: `q = {2^4, 2^8, 2^{16}, 2^{32}-5}`.

The second input is a ``struct`` that controls the use of tracing in the given
object. ``kodo::enable_trace`` or ``kodo::disable_trace`` respectively enables
or disables the tracing functionality in the objects where they are mployed.
For our implementation, we enable tracing for our decoder and disable it for
the encoder. Later in the simulation parameters we will check what options does
tracing has on each device type.

Main simulation class
^^^^^^^^^^^^^^^^^^^^^

We represent our Kodo simulation as a class with different functionalities. Of
course, this is purely subjective. You may choose how you represent your objects
in your simulation. Although, we choose this way because it enabled us to
modularize all the simulation into a single object that controls the system
through the tasks of the devices. Also, other ns-3 objects can extract
information from it in an easy way.

The ``KodoSimulation`` class can be roughly defined in the following way:

.. code:: c++

   class KodoSimulation
   {
   public:

     KodoSimulation(const rlnc_encoder::pointer& encoder,
                    const rlnc_decoder::pointer& decoder)
       : m_encoder(encoder),
         m_decoder(decoder)
     {
       // Constructor
     }

     void ReceivePacket (Ptr<Socket> socket)
     {
       // Receiver actions when a packet is received on its socket
     }

     void GenerateTraffic (Ptr<Socket> socket, Time pktInterval)
     {
       // Transmitter actions performed every "pktInterval" on its socket
     }

   private:

     rlnc_encoder::pointer m_encoder;  // Pointer to encoder
     rlnc_decoder::pointer m_decoder;  // Pointer to decoder

     std::vector<uint8_t> m_payload_buffer; // Buffer for handling current coded packet and its coded coefficients

     uint32_t m_transmission_count;  // Amount of transmissions from the encoder

   };

For the simulation, ``void GenerateTraffic(Ptr<Socket> socket, Time
pktInterval)`` generates coded packets from generic data (created in the
constructor) every ``pktInterval`` units of ``Time`` (which is a ns-3 type) and
sends them to the decoder through its socket connection, represented by the
ns-3 template-based smart pointer object ``Ptr<Socket>``. Several ns-3 objects
are represented in this way. As we will check later, ``void
ReceivePacket(Ptr<Socket> socket)`` will be invoked through a callback whenever
a packet is received at the decoder.

Both sockets make use of ``m_payload_buffer``. The transmitter creates coded
packets from the data and puts them in the buffer. Conversely, a received coded
packet is placed in the buffer and then to the decoding matrix. You can check
the source code to verify that these functionalities are performed by the APIs
``m_encoder->encode()`` and ``m_decoder->decode()``. For the encoding case, the
amount of bytes required from the buffer to store the coded packet and its
coefficients is returned. This amount is needed for the ns-3 ``Create<Packet>``
template-based constructor to create the ns-3 coded packet that is actually sent
(and received).

