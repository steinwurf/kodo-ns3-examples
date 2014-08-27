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
(and received). Finally, ``m_transmission_count`` indicates how many packets
were sent by the encoder during the whole process. Please make a review to
the implementation of ``GenerateTraffic`` and ``ReceivePacket`` to verify the
expected behaviour of the nodes when packets are sent or received respectively.

Default parameters and command parsing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: c++

 int main (int argc, char *argv[])
 {
   std::string phyMode ("DsssRate1Mbps");
   double rss = -93;  // -dBm
   uint32_t packetSize = 1000; // bytes
   double interval = 1.0; // seconds
   uint32_t generationSize = 5;

   CommandLine cmd;

   cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
   cmd.AddValue ("rss", "received signal strength", rss);
   cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
   cmd.AddValue ("interval", "interval (seconds) between packets", interval);
   cmd.AddValue ("generationSize", "Set the generation size to use",
                 generationSize);

   cmd.Parse (argc, argv);

   // Convert to time object
   Time interPacketInterval = Seconds (interval);

The first part of the ``main`` function introduces us to the basic simulation
parameters regarding physical layer mode for WiFi (Direct Sequence Spread
Spectrum of 1 Mbps rate), receiver signal strength of -93 dBm (decibels with
respect to 1 mW of received power), 1 KB for packet size, 1 second interval
duration between ns-3 events (we will use it later) and a generation size of
5 packets. After that, the ``CommandLine`` class is ns-3's command line parser
used to modify those values (if required) with ``AddValue`` and ``Parse``. Then,
the interval duration is converted to the ns-3 ``Time`` format.


Configuration defaults
^^^^^^^^^^^^^^^^^^^^^^

.. code:: c++

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
                      StringValue ("2200"));

  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                      StringValue ("2200"));

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

Before continuing, you will see many features of ns-3's WiFi implementation. So,
a good preview for this can be found `here <http://www.nsnam.org/docs/release/3.20/models/singlehtml/index.html#document-wifi>`_.
Besides the WiFi properties you will find a typical workflow about setting and
configuring WiFi devices in your simulation.

This part basically sets some MAC properties that we will not need (at least for
our purposes), namely frame fragmentation to be applied for frames larger
than 2200 bytes, disabling the RTS/CTS frame collision protocol for the less
than 2200 bytes and setting the broadcast data rate to be the same as unicast
for the given ``phyMode``.

WiFi PHY and channel helpers for nodes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: c++

  // Source and destination
  NodeContainer c;
  c.Create (2);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();

  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) );

  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",
                                  DoubleValue (rss));
  wifiPhy.SetChannel (wifiChannel.Create ());

In this part we start to build the topology for our simulation following
a typical ns-3 workflow. By typical we mean that this can be done in different
ways, but this one you might see regularly within ns-3 simulations. We start by
creating the nodes that we need with the ``NodeContainer`` class. You can create
the nodes separately but this way offers the possibility to easily assign
common properties to the nodes.

We aid ourselves by using the ``WiFiHelper`` class to set the standard to use.
Since we are working with DSSS, this means we need to use IEEE 802.11b. For the
physical layer we use the ``YansWifiPhyHelper::Default()`` constructor and from
it, we disable any gains in the receiver and set the pcap (packet capture)
tracing format at the data link layer. ns-3 supports different formats, here
we picked the `RadioTap <http://www.radiotap.org/>`_ format but you can choose
other format available in the helper description in its Doxygen documentation.
In a similar way, we use the ``YansWifiChannelHelper`` to create our WiFi
channel, where we have set the class property named ``SetPropagationDelay`` to
``ConstantSpeedPropagationDelayMode``. This means that the delay between the
transmitter and the receiver signals is set by their distance between them,
divided by the speed of light. The ``AddPropagationLoss`` defines how do we
calculate the receiver signal strength (received power) in our model. In this
case, we have chosen a ``FixedRssLossModel`` which sets the received power to
a fixed value regardless of the position the nodes have. This fixed value is
set to -93 dBm, but we can modify through argument parsing. With these settings
we create our WiFi PHY layer and channel by doing ``wifiPhy.SetChannel
(wifiChannel.Create ());``. If you want to read more about how the helpers are
implemented, you can check the `Yans description <http://cutebugs.net/files/wns2-yans.pdf>`_
for further details.

WiFi MAC and net device helpers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: c++

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

Now that we have created the physical objects (remember our previous
definition), we proceed to create the network interface cards (NIC, i.e. net
devices) that will communicate the different nodes. But first, we need to set
up the MAC layer. For this we use the ``NqosWifiMacHelper`` which provides an
object factory to create instances of WiFi MACs that do not have
802.11e/WMM-style QoS support enabled. We picked this one because we are just
interested in sending and receiving some dat without QoS. By setting the type
as ``AdhocWifiMac``, we tell ns-3 that the nodes work in a decentralized way.
We also need to set the devices data rate control algorithms, which we do with
the ``WifiHelper`` by setting the remote station manager property to
``ConstantRateWifiManager`` for data and control packets using the given
``phyMode``. This implies that we a fixed data rate for data and control packet
transmissions. With all the previous settings we create our (2) WiFi cards
and put them in a container by doing
``NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);``

Mobility model and helper
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: c++

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc =
    CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);

The ns-3 ``MobilityHelper`` class assigns a model for the velocities of the
within ns-3. Even though we had fixed the received power of the decoder, it is
a necessary component for the ``YansWiFiChannelHelper``. We create a ``Vector``
describing the initial (and remaining) coordinates for both transmitter and
receiver in a 3D grid. Then, we put them in the helper with a
``ConstantPositionMobilityModel`` for the nodes.

Internet and application protocol helpers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: c++

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

After we have set up the devices and the two lowest layers, we need to set up
the network and application layer protocols. The ``InternetStackHelper``
provides functionalities for IPv4, ARP, UDP, TCP, IPv6, Neighbor Discovery, and
other related protocols. You can find more about the implementation of the
helper `here <http://www.nsnam.org/docs/release/3.20/models/singlehtml/index.html#document-internet-models>`_.
A similar process is made for the IPv4 address assignment. We use the address
range ``10.1.1.0`` with the subnet mask ``255.255.255.0`` we assign it to the
``devices`` and put the result in a container.

Simulation calls
^^^^^^^^^^^^^^^^

.. code:: c++

  rlnc_encoder::factory encoder_factory(generationSize, packetSize);
  rlnc_decoder::factory decoder_factory(generationSize, packetSize);

  KodoSimulation kodoSimulator(encoder_factory.build(),
                               decoder_factory.build());

With previous defined typedefs, we call the encoder and decoder factory to
set and generate object with the defined inputs. Then, we create the instances
with ``encoder_factory.build()`` and ``decoder_factory.build()`` to call the
simulation class constructor. This does not run the simulation as we will see,
but it creates the objets called by ns-3 to perform the tasks of the transmitter
and receiver.

Socket creation and connections
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: c++

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&KodoSimulation::ReceivePacket,
                                           &kodoSimulator));

  Ptr<Socket> source = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"),
                                                80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

For the application protocols to work with a given data, we need a pair between
an IP address and a logical port to create a socket address for socket
communication (besides of course, the socket itself). ns-3 supports two sockets
API for user space applications. The first is ns-3 native, while the second
(which is based on the first) resembles more a real system POSIX-like socket
API. The differences between the two can be found
`here <http://www.nsnam.org/docs/release/3.20/models/singlehtml/index.html#document-network>`_.
We will focus on the ns-3 socket API variant.

The first two lines are meant to create the socket type from a lookup search
given by the name ``UdpSocketFactory`` and create this type of socket on the
receiver and the transmitter
