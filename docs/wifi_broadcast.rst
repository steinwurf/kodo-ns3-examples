Broadcast RLNC with a WiFi channel
==================================

.. _wifi_broadcast:

General topology
----------------

The topology considered describes a transmitter sending coded packets
with RLNC from a generation size :math:`g` and field size :math:`q` in a
broadcast fashion through a 802.11b channel to N receivers. For the
purpose of our example, we will start with :math:`g = 5`, :math:`q = 2`
(i.e. the binary field), :math:`N = 2` and we will check the completion
time in terms of transmissions through the WiFi channel under different
situations. Topology is shown as follows:

.. literalinclude:: ../src/wifi_broadcast/main.cc
   :language: c++
   :start-after: //! [0]
   :end-before: //! [1]
   :linenos:

What to simulate?
-----------------

We will consider the following guidelines for our simulation:

* Behaviour: The sender keeps transmitting the generation until the
  receiver has :math:`g` linearly independent (l.i.) coded packets
  (combinations). Packets might or might not be loss due to channel
  impairments.
* Inputs: As main parameters regarding RLNC, we choose the generation
  and field size. A parameter regarding channel control should be included
* Outputs: A counter to indicate how much transmissions did the process
  required and some prints to indicate when decoding is completed.
* Scenarios: We will variate the generation and field size to verify
  theoretical expected values regarding the amount of transmissions to
  decode. Also, the number of transmissions should somehow change as we
  vary the channel.

Program description
-------------------

After the project has been properly configured and built, you should have
a folder named ``~/dev/kodo-ns3-examples/src/wifi_broadcast/`` where ``~/dev/``
is the folder where you cloned the project repository. If you check the
``wifi_broadcast`` folder, you will see the ``main.cc`` file which contains
the source code of this simulation. You can open it with your preferred editor
to review the source code. We will briefly review some of its parts.

Overview comments and includes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. literalinclude:: ../src/wifi_broadcast/main.cc
   :language: c++
   :start-after: //! [2]
   :end-before: //! [3]
   :linenos:

As with any source code, the overview comments provide a reference to the users
regarding project general aspects. The E-macs descriptor is part of the
ns-3 coding style to allow E-macs developers' editor to recognize the document
type. Following, licensing terms and an introduction to what we are simulating
are displayed. Header includes are ordered from most general to particular functionalities within ns-3 and Kodo. From ns-3, the necessary modules are:

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
  objects in the examples.
* Mobility module: For providing a description of how the nodes move in an
  environment. We will use briefly this module for a representation of our
  physical channel.
* Config-store module: A specialized ns-3 database for internal attributes and
  default values for the different APIs.
* WiFi module: A PHY and MAC layer models for WiFi. Necessary for our medium
  access control and channel model.
* Internet module: For handling the IPv4 protocol at the network layer.

Other includes are particular to this implementation and they can be found in
standard C++ code. From Kodo, the header ``broadcast-rlnc.h`` contains the description of the encoder and decoder objects we use.

Default Namespace
^^^^^^^^^^^^^^^^^

We will be working within the ns-3 scope given that most of our objects are from
this library. This is typical across ns-3 code.

.. code-block:: c++

   using namespace ns3;

Main simulation class
^^^^^^^^^^^^^^^^^^^^^

Before starting, we describe a Kodo object created with the purpose to
represent the RLNC broadcast topology. In this sense, we represent our Kodo simulation as a class with different functionalities. Of course, this is purely subjective. You may choose how you represent your objects in your simulation. Although, we choose this way because it enabled us to modularize all the
simulation into a single object which is controlled by the network through the
tasks of the devices. Also, other ns-3 objects can extract information
from it in an easy way.

The ``BroadcastRlnc`` class can be roughly defined in the following way:

.. code-block:: c++

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
       // Receiver actions performed when a packet is received on its socket
     }

     void GenerateTraffic (Ptr<Socket> socket, Time pktInterval)
     {
       // Transmitter actions performed every "pktInterval" on its socket
     }

   private:

     rlnc_encoder::pointer m_encoder;  // Pointer to encoder
     rlnc_decoder::pointer m_decoder;  // Pointer to decoder

     std::vector<uint8_t> m_payload_buffer; // Buffer for handling current
                                            // coded packet and its coefficients

     uint32_t m_transmission_count;  // Amount of transmissions from the encoder

   };

For the simulation, ``void GenerateTraffic(Ptr<Socket> socket, Time
pktInterval)`` generates coded packets from generic data (created in the
constructor) every ``pktInterval`` units of ``Time`` (which is a ns-3 type) and
sends them to the decoder through its socket connection, represented by the
ns-3 template-based smart pointer object ``Ptr<Socket>``. Several ns-3 objects
are represented in this way. So quite often you will see this kind of pointer
employed. This type of pointer is made to make a proper memory usage.

As we will check later, ``void ReceivePacket(Ptr<Socket> socket)`` will be
invoked through a callback whenever a packet is received at the decoder. Both
sockets make use of ``m_payload_buffer``. The transmitter creates coded
packets from the data and puts them in the buffer. Conversely, a received coded
packet is placed in the buffer and then to the decoding matrix.

You can check the source code to verify that these functionalities are
performed by the APIs ``m_encoder->encode()`` and ``m_decoder->decode()``. For
the encoding case, the amount of bytes required from the buffer to store the
coded packet and its coefficients is returned. This amount is needed for the
ns-3 ``Create<Packet>`` template-based constructor to create the ns-3 coded
packet that is actually sent (and received). Finally, ``m_transmission_count``
indicates how many packets were sent by the encoder during the whole process.
Please make a review to the implementation of ``GenerateTraffic`` and
``ReceivePacket`` to verify the expected behaviour of the nodes when packets
are sent or received respectively.

Default parameters and command parsing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

 int main (int argc, char *argv[])
 {
   std::string phyMode ("DsssRate1Mbps");
   double rss = -93;  // dBm
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
Spectrum of 1 Mbps rate), receiver signal strength of -93 dBm, 1 KB for packet
size, 1 second interval duration between ns-3 events (we will use it later) and
a generation size of 5 packets. After that, the ``CommandLine`` class is ns-3's
command line parser used to modify those values (if required) with ``AddValue``
and ``Parse``. Then, the interval duration is converted to the ns-3 ``Time``
format.


Configuration defaults
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
                      StringValue ("2200"));

  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                      StringValue ("2200"));

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

Before continuing, you will see many features of ns-3's `WiFi implementation
<http://www.nsnam.org/docs/release/3.20/models/singlehtml/index.html#document-wifi>`_.
Besides the WiFi properties, in the previous link you will find a typical
workflow about setting and configuring WiFi devices in your simulation.

This part basically sets off some MAC properties that we do not need (at least
for our purposes), namely frame fragmentation to be applied for frames larger
than 2200 bytes, disabling the RTS/CTS frame collision protocol for the less
than 2200 bytes and setting the broadcast data rate to be the same as unicast
for the given ``phyMode``. However, they need to be included in order to work
with the WiFi MAC.

WiFi PHY and channel helpers for nodes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

  // Source and destination
  NodeContainer c;
  c.Create (2);

  // The below set of helpers will help us to put together the WiFi NICs we want
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
physical layer, we use the ``YansWifiPhyHelper::Default()`` constructor and from
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
set to -93 dBm, but we can modify it through argument parsing. With these
settings, we create our WiFi PHY layer channel by doing ``wifiPhy.SetChannel
(wifiChannel.Create ());``. If you want to read more about how the helpers are
implemented, you can check the `Yans description <http://cutebugs.net/files/wns2-yans.pdf>`_
for further details.

WiFi MAC and net device helpers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

Now that we have created the physical objects (the nodes, remember our previous
definition), we proceed to create the network interface cards (NICs, i.e. net
devices) that will communicate the different nodes. But first, we need to set
up the MAC layer. For this, we use the ``NqosWifiMacHelper`` which provides an
object factory to create instances of WiFi MACs, that do not have
802.11e/WMM-style QoS support enabled. We picked this one because we are just
interested in sending and receiving some data without QoS. By setting the type
as ``AdhocWifiMac``, we tell ns-3 that the nodes work in a decentralized way.
We also need to set the devices data rate control algorithms, which we do with
the ``WifiHelper``. This is achieved by setting the remote station manager
property to ``ConstantRateWifiManager`` for data and control packets using the
given ``phyMode``. This implies that we a fixed data rate for data and control
packet transmissions. With all the previous settings we create our (2) WiFi
cards and put them in a container by doing
``NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);``.

Mobility model and helper
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc =
    CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);

The ns-3 ``MobilityHelper`` class assigns a model for the velocities of
the receivers within ns-3. Even though we had fixed the received power of the
decoder, it is a necessary component for the ``YansWiFiChannelHelper``. We
create a ``Vector`` describing the initial (and remaining) coordinates for both
transmitter and receiver in a 3D grid. Then, we put them in the helper with a
``ConstantPositionMobilityModel`` for the nodes.

Internet and application protocol helpers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

After we have set up the devices and the two lowest layers, we need to set up
the network and application layer protocols. The ``InternetStackHelper``
provides functionalities for IPv4, ARP, UDP, TCP, IPv6, Neighbor Discovery, and
other related protocols. You can find more about the implementation of the
helper in this `link <http://www.nsnam.org/docs/release/3.20/models/singlehtml/index.html#document-internet-models>`_.
A similar process is made for the IPv4 address assignment. We use the address
range ``10.1.1.0`` with the subnet mask ``255.255.255.0``, assign it to the
``devices`` and put the result in a container.

Simulation calls
^^^^^^^^^^^^^^^^

.. code-block:: c++

  rlnc_encoder::factory encoder_factory(generationSize, packetSize);
  rlnc_decoder::factory decoder_factory(generationSize, packetSize);

  KodoSimulation kodoSimulator(encoder_factory.build(),
                               decoder_factory.build());

With previous defined typedefs, we call the encoder and decoder factories to
set and generate objects with the defined inputs. Then, we create the instances
with ``encoder_factory.build()`` and ``decoder_factory.build()`` to call the
simulation class constructor. This does not run the simulation as we will see,
but it creates the objets called by ns-3 to perform the tasks of the transmitter
and receiver.

Socket creation and connections
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

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
APIs for user space applications. The first is ns-3 native, while the second
(which is based on the first) resembles more a real system POSIX-like socket
API. For further information about the differences, please refer to ns-3's
`socket implementation <http://www.nsnam.org/docs/release/3.20/models/singlehtml/index.html#document-network>`_.
We will focus on the ns-3 socket API variant.

The first two lines are meant to create the socket type from a lookup search
given by the name ``UdpSocketFactory``. They create this type of socket on the
receiver and the transmitter. We have chosen the previous socket type in order
to represent a UDP connection that sends RLNC coded packets. Then, we create
the local socket address for binding purposes. For it, we choose the default
``0.0.0.0`` address obtained from ``Ipv4Address::GetAny ()`` and port 80 (to
represent random HTTP traffic). The receiver binds to this address for socket
hearing. Everytime a packet is received we trigger a callback to the reference
``&KodoSimulation::ReceivePacket`` which takes the hearing socket as an argument.
This executes the respective member function of the reference ``&kodoSimulator``.
For the transmitter (source) we make a similar process but instead we allow
broadcasting with ``source->SetAllowBroadcast (true)`` and connect to the
broadcast address. This completes our socket connection process and links the
pieces for the simulation.

Simulation event handler
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

  // Pcap tracing
  wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0),
                                  &KodoSimulation::GenerateTraffic,
                                  &kodoSimulator,
                                  source, interPacketInterval);

  Simulator::Run ();
  Simulator::Destroy ();

Finally, ``wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);`` allows the net
devices to create pcap files from the given devices. One file per net device.
File naming will be: ``wifi-simple-adhoc-[NODE_ID]-[DEVICE_ID].pcap`` and the
format of these files should be the one of RadioTap and should be located on your
``~/kodo-ns3-examples/`` folder. Later, we will review how to read those files.

After the pcap setting, we use one of the ns-3 core features, event scheduling.
The ``Simulator`` is inherent to ns-3 and defines how events are handled
discretely. The ``ScheduleWithContext`` member function basically tells ns-3
to schedule the ``KodoSimulation::GenerateTraffic`` function every second from
the transmitter instance of ``kodoSimulator`` and provide its arguments, e.g.
ns-3 socket pointer ``source`` and ``Time`` packet interval
``interPacketInterval``. Among the event schedulers, you will see ``Schedule``
vs. ``ScheduleWithContext``. The main difference between these two functions
is that the ``ScheduleWithContext`` tells ns-3 that the scheduled's event
context (the node identifier of the currently executed network node) belongs
to the given node. Meanwhile, ``Schedule`` may receive the context from a
previous scheduled event, which can have the context from a different node.
You can find more details about  the simulator functions in the ns-3
`event scheduling <http://www.nsnam.org/docs/manual/singlehtml/index.html#document-events>`_
manual. With all previous descriptions, we are able to run the simulation to
see some basic effects of network coding in ns-3 with Kodo.

Simulation runs
---------------

Now that we know each part of our setup, we will run some simulations in order
that you should know what to expect. We will run the default behaviour and
change some parameters to check known results.

Default run
^^^^^^^^^^^

First type ``cd ~/dev/kodo-ns3-examples`` in your terminal for you to be in
the main path of your cloned repository. Remember that at this point, **you need
to have configured and built the projects with no errors**. The default run goes
with 5 packets in the binary field with only the decoder trace enabled. For the
trace, we have only set ``input_symbol_coefficients`` to see the coding
coefficients of a received packet and ``decoder_state`` to see how the state
matrix evolves. As a starter, type: ::

  ./build/linux/wifi_broadcast/wifi_broadcast

You should see an output similar to this: ::

  Received one packet at decoder
  Trace decoder:
  input_symbol_coefficients:
  C: 0 1 0 1 0

  decoder_state:
  000 ?:  0 0 0 0 0
  001 C:  0 1 0 1 0
  002 ?:  0 0 0 0 0
  003 ?:  0 0 0 0 0
  004 ?:  0 0 0 0 0

  Received one packet at decoder
  Trace decoder:
  input_symbol_coefficients:
  C: 0 0 0 1 0

  decoder_state:
  000 ?:  0 0 0 0 0
  001 C:  0 1 0 0 0
  002 ?:  0 0 0 0 0
  003 C:  0 0 0 1 0
  004 ?:  0 0 0 0 0

  Received one packet at decoder
  Trace decoder:
  input_symbol_coefficients:
  C: 1 0 0 0 1

  decoder_state:
  000 C:  1 0 0 0 1
  001 C:  0 1 0 0 0
  002 ?:  0 0 0 0 0
  003 C:  0 0 0 1 0
  004 ?:  0 0 0 0 0

  Received one packet at decoder
  Trace decoder:
  input_symbol_coefficients:
  C: 1 0 1 0 0

  decoder_state:
  000 C:  1 0 0 0 1
  001 C:  0 1 0 0 0
  002 C:  0 0 1 0 1
  003 C:  0 0 0 1 0
  004 ?:  0 0 0 0 0

  Received one packet at decoder
  Trace decoder:
  input_symbol_coefficients:
  C: 0 1 0 0 1

  decoder_state:
  000 U:  1 0 0 0 0
  001 U:  0 1 0 0 0
  002 U:  0 0 1 0 0
  003 U:  0 0 0 1 0
  004 U:  0 0 0 0 1

  Decoding completed! Total transmissions: 5

Here we observe that everytime a packet is received, the previously
mentioned information is printed. For the ``input_symbols_coefficients`` output,
``C:`` indicates that we have a received a *coded* packet with the given
coding vector. In this output, the first given coded packet (CP) is:
:math:`CP_1 = p_2 + p_4`.

.. note:: Normally the ``rlnc_encoder`` type (based on the
   ``full_rlnc_encoder``), would have generated packets in a systematic way,
   but here we set that feature off in the ``KodoSimulation`` class constructor,
   through the encoder API ``m_encoder->set_systematic_off()``. Also, normally
   the encoder starts with the same seed in every run but have also changed that
   too in the constructor with ``m_encoder->seed(time(0))``. So, we proceed
   with this example to explain the simulation, but you will obtain another
   result in your runs. However, the results obtained with this example apply in
   general.

After the input symbols have been checked, the decoder trace shows the
``decoder_state``. This is the current decoding matrix in an equivalent row
echelon form. Given that we have received :math:`p_2 + p_4`, we put them in the
second row because the pivot for :math:`p_2` is there. Also, we can argue that
the pivot for :math:`p_1` is in first row and so on. The second received coded
packet is :math:`CP_2 = p_4`. Notice that when we print the decoder state
again, we have changed the equation of the second row because with the current
information we can calculate :math:`p_2 = CP_1 + CP_2` (remember we are in
modulo-2 arithmetic). However, we still keep these values as "coded" (``C:``),
because we need to receive the complete generation to guarantee full decoding.
Packet reception continues until we have :math:`g` linearly independent (l.i.)
coded packets.

You can also see there two more types of symbols indicators. ``?:`` indicates
that the corresponding pivot packet has not been *seen* by the decoder. Seeing
packet :math:`k` means that we are able to compute :math:`p_k
+ \sum_{l > k} \alpha_l p_l`, i.e. to be able to compute :math:`p_k` plus a
combinations of packets of indexes greater than :math:`k`. Even though it seems
simple and unrelated, the concept of seeing a packet will prove to be useful in
future examples. Finally, ``U:`` indicates that the packet is uncoded, normally
you will see this when the complete generation is decoded.

At the end, we see that decoding was performed after 5 transmissions. There are
two reasons for this to occur. First, no linearly dependent (l.d.)
combinations ocurred during the random process. Second, there were no packet
erasures neither. We will make some changes to see these effects.

Changing the field and generation size
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Try to run the example again several times, you should see that the amount of
tranmissions vary between 5 and 7, maybe sometimes a little more, due to
randomness. On average, for :math:`q = 2` you should expect that
:math:`g + 1.6` transmissions are necessary to transmit :math:`g` l.i.
packets. To verify this, you can save the following bash script as
``extra_packet_per_generation.bash`` in your ``~/dev/kodo-ns3-examples``:

.. code-block:: bash

   #!/bin/bash

   #Check the number of extra transmission per generation

   SUM=0
   N=$1  # Number of runs
   GENERATION_SIZE=$2  #  Generation size

   #  For-loop with range for bash to run the experiment many times
   #  and collect the total transmissions to get the average

   for (( c=1; c<=${N}; c++ ))
   do
       COMB=`./build/linux/wifi_broadcast/wifi_broadcast | \
             grep "Total transmissions:" | cut -f5 -d\ `
       SUM=$(( ${SUM} + ${COMB} ))
   done

   EXTRA=`echo "scale= 4; (${SUM} / ${N}) - ${GENERATION_SIZE}" | bc`

   echo "Extra packets per generation: ${EXTRA}"

To set the permissions for this file, type in type in your
terminal: ::

   chmod 755 extra_packet_per_generation.bash

This enables you and others to run and read the script, but only you to write it.
You can set this according to the needs in your system. For further permissions,
you can refer to the ``chmod`` instruction for Unix-like systems.

The script receives two arguments: numbers of runs and generation size.
Basically it returns how much extra packets per generation were necessary for
decoding. Try to running as follows: ::

   ./extra_packet_per_generation.bash 100 5
   Extra packets per generation: .9400
   ./extra_packet_per_generation.bash 1000 5
   Extra packets per generation: 1.4790
   ./extra_packet_per_generation.bash 10000 5
   Extra packets per generation: 1.5657

You can see that as we increase the amount of runs, we approach to 1.6 extra
packets per generation. This is due to the linear dependency process of the
coded packets. However, this happens because we are using the binary field.
Set the field to :math:`q = 2^8` by setting ``fifi::binary8`` in the encoder
and decoder templates, rebuild the project (by typing again ``./waf build`` in
your ``~/dev/kodo-ns3-examples`` folder) and rerun the script even with 100
samples, to see that the amount of extra packets is zero (at least with 4
decimal places). This is because it is very unlikely to receive linearly
dependent packets, even when the last coded packet is being sent.

To see the new coding coefficients for :math:`q = 2^8`, but for only a
generation size of 3 packets, type now: ::

  ./build/linux/wifi_broadcast/wifi_broadcast --generationSize=3

You should see something similar to: ::

  Received one packet at decoder
  Trace decoder:
  input_symbol_coefficients:
  C: 224 129 0

  decoder_state:
  000 C:  1 198 0
  001 ?:  0 0 0
  002 ?:  0 0 0

  Received one packet at decoder
  Trace decoder:
  input_symbol_coefficients:
  C: 159 115 75

  decoder_state:
  000 C:  1 0 56
  001 C:  0 1 74
  002 ?:  0 0 0

  Received one packet at decoder
  Trace decoder:
  input_symbol_coefficients:
  C: 240 92 115

  decoder_state:
  000 U:  1 0 0
  001 U:  0 1 0
  002 U:  0 0 1

  Decoding completed! Total transmissions: 3

Notice how the size of the decoding matrix changes due to the effect of the
generation size. This is expected because the size of the decoding matrix is
given by the minimum amount of linear combinations required to decode. Also, you
can verify the coding coefficients now vary between 0 and 255 given that we
have changed the field size. Try running the example with these changes a
couple of times so you can verify the above in general.

Changing the receiver signal strength
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As we mentioned earlier, our WiFi PHY layer relies on constant position and
power values. We originally set up the ``rss`` value to -93 dBm to indicate our
received power. In general, packet error rate varies with the signal
reception level, so we will adjust this. The receiver sensitivity for this
channel is -96 dBm. It means that for ``rss`` values lower than this, we will
have no packet recovery. This goes a little further from a typical erasure
channel where we may or may not have packet losses regurlarly, the reason being
that receiver position and received power are fixed.

To change the ``rss`` value , simply type: ::

  ./build/linux/wifi_broadcast/wifi_broadcast --rss=-96

You will see no output because the program gets into an infinite loop. To finish
the program type ``Ctrl+C`` in your terminal. To verify that the running
program endend properly, verify that a ``^C`` sign appears in your terminal. The
program enters a loop because we receive no packets at all and the decoder will
never be full rank.

Using other tracing features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

So far we have seen only the decoder state in terms of rank and symbol
coefficients. In the ``filters`` construct on the ``ReceivePacket``  function
in the ``main.cpp`` file, you can add the ``"symbol_storage"`` option to see a
hex dump of the packets. Do so, save your files, rebuild and type: ::

  ./build/linux/wifi_broadcast/wifi_broadcast --generationSize=3

Then, you will get an output like this (here we used the binary field): ::

  Received one packet at decoder
  Trace decoder:
  input_symbol_coefficients:
  C: 0 1 1

  decoder_state:
  000 ?:  0 0 0
  001 C:  0 1 1
  002 ?:  0 0 0

  symbol_storage:
  0 A:
  0000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  0010  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  ....
  03e0
  1 I:
  0000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  0010  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  ....
  03e0
  2 A:
  0000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  0010  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  ....
  03e0

  Received one packet at decoder
  Trace decoder:
  input_symbol_coefficients:
  C: 0 0 1

  decoder_state:
  000 ?:  0 0 0
  001 C:  0 1 0
  002 C:  0 0 1

  symbol_storage:
  0 A:
  0000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  0010  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  ....
  03e0
  1 I:
  0000  78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78  xxxxxxxxxxxxxxxx
  0010  78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78  xxxxxxxxxxxxxxxx
  ....
  03e0
  2 I:
  0000  78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78  xxxxxxxxxxxxxxxx
  0010  78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78  xxxxxxxxxxxxxxxx
  ....
  03e0

  Received one packet at decoder
  Trace decoder:
  input_symbol_coefficients:
  C: 1 0 1

  decoder_state:
  000 U:  1 0 0
  001 U:  0 1 0
  002 U:  0 0 1

  symbol_storage:
  0 I:
  0000  78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78  xxxxxxxxxxxxxxxx
  0010  78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78  xxxxxxxxxxxxxxxx
  ....
  03e0
  1 I:
  0000  78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78  xxxxxxxxxxxxxxxx
  0010  78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78  xxxxxxxxxxxxxxxx
  ....
  03e0
  2 I:
  0000  78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78  xxxxxxxxxxxxxxxx
  0010  78 78 78 78 78 78 78 78 78 78 78 78 78 78 78 78  xxxxxxxxxxxxxxxx
  ....
  03e0

  Decoding completed! Total transmissions: 5

Now, we see the data in rows of 16 bytes. If you look at the constructor in
``main.cpp``, you can confirm that we constantly fill the buffer with ``x``,
since the example is just for showing purposes.

The symbol storage can be mainly in 3 states depending on how the memory is
assigned in Kodo. In the library we have 2 types of memory assigment for object
creation, i.e. we can create a `shallow copy or a deep copy <http://stackoverflow.com/a/184745>`_.
For this implementation, we use a deep copy by default and we will only have 2
of them, namely ``A:`` (available) and ``I:`` (initiliazed) meaning that the
memory is ready and initialized to be used, respectively. Notice that whenever
we still have coded packets, we only print zeros. In the case of a shallow
copy, we might see the ``?:`` indicator that will tell us that the storage
has not been assigned. This trace feature is useful particularly we you want to
debug the decoding process with some known data.

Finally, try disabling the decoder trace and enable the encoder trace. This
trace only has the symbol storage feature. Simply switch the structs in the
encoder and decoder templates, save, rebuild your project and rerun the example
with the previous setting, you will only see your data in the encoder.

Review pcap traces
^^^^^^^^^^^^^^^^^^

As we described earlier, the simulation leaves pcap format files
(``wifi-simple-adhoc-*-*.pcap``) in your ``~/dev/kodo-ns3-examples`` folder.
You can read these files with different programs like tcpdump or Wireshark.
tcpdump is standard on most Unix-like systems and is based on the libpcap
library. `Wireshark <https://www.wireshark.org/>`_ is another free, open-source
packet analyzer which you can get online. Just for showing purposes we will use
tcpdump, but you can choose the one you prefer the most. For reading both files,
simply type: ::

  tcpdump -r wifi-simple-adhoc-0-0.pcap -nn -tt
  tcpdump -r wifi-simple-adhoc-1-0.pcap -nn -tt

You will get this output (it will look different on your terminal): ::

  reading from file wifi-simple-adhoc-0-0.pcap, link-type IEEE802_11_RADIO
  (802.11 plus radiotap header)
  1.000000 1000000us tsft 1.0 Mb/s 2412 MHz 11b IP 10.1.1.1.49153 >
  10.1.1.255.80: UDP, length 1002
  2.000000 2000000us tsft 1.0 Mb/s 2412 MHz 11b IP 10.1.1.1.49153 >
  10.1.1.255.80: UDP, length 1002
  3.000000 3000000us tsft 1.0 Mb/s 2412 MHz 11b IP 10.1.1.1.49153 >
  10.1.1.255.80: UDP, length 1002
  4.000000 4000000us tsft 1.0 Mb/s 2412 MHz 11b IP 10.1.1.1.49153 >
  10.1.1.255.80: UDP, length 1002

  reading from file wifi-simple-adhoc-1-0.pcap, link-type IEEE802_11_RADIO
  (802.11 plus radiotap header)
  1.008720 1008720us tsft 1.0 Mb/s 2412 MHz 11b -93dB signal -101dB noise
  IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1002
  2.008720 2008720us tsft 1.0 Mb/s 2412 MHz 11b -93dB signal -101dB noise
  IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1002
  3.008720 3008720us tsft 1.0 Mb/s 2412 MHz 11b -93dB signal -101dB noise
  IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1002
  4.008720 4008720us tsft 1.0 Mb/s 2412 MHz 11b -93dB signal -101dB noise
  IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1002

The ``0-0`` file stands for the encoder net device and ``1-0`` for the receiver
net device. There you can confirm the RadioTap format of the pcap files and
also can check other features like bit rate, frequency channel, protocol used,
rss, noise floor and the transmitter and receiver IP addresses with their
respective ports. Notice that these fit with our settings configuration.
