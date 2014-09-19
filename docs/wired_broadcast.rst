Broadcast RLNC with a 2-user erasure channel
============================================

.. _wired_broadcast:

This example is similar to the first example, but now the channel will
be modeled with an erasure rate and packets are broadcasted to 2 users
instead of one. In this way, the topology considered describes a transmitter
sending coded packets with RLNC from a generation size :math:`g` and field size
:math:`q` in a broadcast erasure channel to 2 receivers. As with previous the
example, we will start with :math:`g = 5` and :math:`q = 2` and we will again
observe completion time in terms of transmissions. Although, now we include
the erasure rate (in percentage), :math:`0 \leq \epsilon < 1`, to indicate
packet losses. For this case, we will assume that both links have the same
erasure rate for simplicity, :math:`\epsilon = 0.3`, i.e. 30% packet losses.

What to simulate?
-----------------

* Behaviour: The sender keeps transmitting the generation until both
  receivers has :math:`g` linearly independent (l.i.) coded packets.
  Packets might or might not be loss at the given rate.
* Inputs: Main parameters will be generation size, field size and packet loss
  rate.
* Outputs: A counter to indicate how much transmissions did the process
  required and some prints to indicate when decoding is completed.
* Scenarios: We will variate the generation and field size to verify
  theoretical expected values regarding the amount of transmissions to
  decode. Also, the number of transmissions should somehow change as we
  vary the channel.

Program description
-------------------

In your local repository, you should have a folder named ``wired_broadcast/``.
If you check it, you will see the ``main.cpp`` file which contains
the source code of this simulation. Its structure is similar to the previous
one, so now we will focus on the main differences.

Main simulation class
^^^^^^^^^^^^^^^^^^^^^

As expected, the main simulation class must be modified in order to now include
the functionalities of each receiver. The modifications in its body are shown
as follows:

.. code-block:: c++

  class KodoSimulation
  {
  public:

    KodoSimulation(const rlnc_encoder::pointer& encoder,
                   const rlnc_decoder::pointer& decoder1,
                   const rlnc_decoder::pointer& decoder2)
      : m_encoder(encoder),
        m_decoder_1(decoder1),
        m_decoder_2(decoder2)
    {
        // Constructor
    }

    void ReceivePacket1 (Ptr<Socket> socket)
    {
        // Receiver 1 actions performed when a packet is received on its socket
    }

    void ReceivePacket2 (Ptr<Socket> socket)
    {
        // Receiver 2 actions performed when a packet is received on its socket
    }

    void GenerateTraffic (Ptr<Socket> socket, Time pktInterval )
    {
        // Transmitter actions performed every "pktInterval" on its socket
    }

  private:

    rlnc_encoder::pointer m_encoder;  // Pointer to encoder
    rlnc_decoder::pointer m_decoder_1;  // Pointer to decoder 1
    rlnc_decoder::pointer m_decoder_2;  // Pointer to decoder 2

    std::vector<uint8_t> m_payload_buffer; // Buffer for handling current
                                           // coded packet and its coefficients
    uint32_t m_transmission_count;
  };

The main difference with the previous simulation is that now we define a packet
reception function for each receiver. In the source code, you will notice that
we have 2 instances of the decoder and a new stopping condition for the
transmitter, e.g. to verify that both receivers are full rank. Otherwise,
this part is the same as the respective one in the first example .

Default parameters and command parsing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For the default parameters, we show what has been added for the erasure rate:

.. code-block:: c++

 int main (int argc, char *argv[])
 {
   // Main parameters
   double errorRate = 0.3; // Error rate for all the links

   // Command parsing
   cmd.AddValue ("errorRate", "Packet erasure rate for the links", errorRate);

Configuration defaults
^^^^^^^^^^^^^^^^^^^^^^

For this part, there are some changes because we have removed the WiFi protocol
and we have represented our channel as a packet erasure channel. This implies to
set a parameter for our error model. We employ the ``RateErrorModel`` class to
implement this model and for it, we need to set up the error rate unit. This
tells ns-3 on which datatype it should apply errors. For our case, we are
interested that it occurs on packets (instead of bits), so we set it up by
doing the following:

.. code-block:: c++

  Config::SetDefault ("ns3::RateErrorModel::ErrorUnit",
                      StringValue ("ERROR_UNIT_PACKET"));


Topology and net helpers
^^^^^^^^^^^^^^^^^^^^^^^^

For creating the topology, we proceed in a different way than the used for the
first example. We use the ``PointToPointHelper`` to create a point-to-point
link. Then, we create the links from the source to each receiver using the
``PointToPointStarHelper`` which takes as an input the desired number of links
and a ``PointToPointHelper`` instance, namely ``pointToPoint`` in our case.
After that, we create the error rate model for each net device in the topology
and enable them. Finally, we set up the Internet stack and IP addresses to our
topology.


.. code-block:: c++

  // Set the basic helper for a single link
  PointToPointHelper pointToPoint;

  // Two receivers against a centralized hub
  PointToPointStarHelper star (2, pointToPoint);

  Ptr<RateErrorModel> error_model = CreateObject<RateErrorModel> ();
  error_model->SetAttribute ("ErrorRate", DoubleValue (errorRate));

  star.GetSpokeNode (0)->GetDevice (0)->
    SetAttribute ("ReceiveErrorModel", PointerValue (error_model));
  star.GetSpokeNode (1)->GetDevice (0)->
    SetAttribute ("ReceiveErrorModel", PointerValue (error_model));
  error_model->Enable ();

  // Setting IP protocol stack
  InternetStackHelper internet;
  star.InstallStack(internet);

  // Set IP addresses
  star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

Socket connections, callback settings and pcap tracing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The socket connections does not differ too much from the first example. The only
difference is that each callback now points to the respective ``ReceivePacket``
member class function. Also, we have enable the population of the routing
tables through ``Ipv4GlobalRoutingHelper::PopulateRoutingTables()`` and again
configured the pcap tracing by doing ``pointToPoint.EnablePcapAll ("star")``.

.. code-block:: c++


  Ptr<Soccket> recvSink1 = Socket::CreateSocket (star.GetSpokeNode (0), tid);
  recvSink1->Bind (local);
  recvSink1->SetRecvCallback (MakeCallback (&KodoSimulation::ReceivePacket1,
                                            &kodoSimulator));

  Ptr<Socket> recvSink2 = Socket::CreateSocket (star.GetSpokeNode (1), tid);
  recvSink2->Bind (local);
  recvSink2->SetRecvCallback (MakeCallback (&KodoSimulation::ReceivePacket2,
                                            &kodoSimulator));

  // Sender
  Ptr<Socket> source = Socket::CreateSocket (star.GetHub (), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"),
                                               port);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Turn on global static routing so we can actually be routed across the star
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Do pcap tracing on all point-to-point devices on all nodes
  pointToPoint.EnablePcapAll ("star");

Simulations runs
----------------

After building and configuring the project, run the example by typing....
