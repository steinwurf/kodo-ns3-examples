Broadcast RLNC with a 2-user erasure channel
============================================

.. _wired_broadcast:

This example is similar to the first example, but now the channel will
be modeled with an erasure rate and packets are broadcasted to 2 users
instead of one. In this way, the topology considered describes a transmitter
sending coded packets with RLNC from a generation size :math:`g` and field size
:math:`q` in a broadcast erasure channel to 2 receivers. As with the previous
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
  required and some prints to indicate when decoding is completed. The
  number of transmissions should change as we vary the input parameters.
* Scenarios: We will variate the generation and field size to verify
  theoretical expected values regarding the amount of transmissions to
  decode.

Program description
-------------------

In your local repository, you should have a folder named ``wired_broadcast/``.
If you check it, you will see the ``main.cpp`` file which contains
the source code of this simulation. Its structure is similar to the previous
one, so now we will focus on the main differences.

Main simulation class
^^^^^^^^^^^^^^^^^^^^^

The main simulation class must be modified in order to now include the
functionalities of each receiver. The modifications in its body are shown
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

  Ptr<RateErrorModel> errorModel1 = CreateObject<RateErrorModel> ();
  errorModel1->SetAttribute ("ErrorRate", DoubleValue (errorRate));

  Ptr<RateErrorModel> errorModel2 = CreateObject<RateErrorModel> ();
  errorModel2->SetAttribute ("ErrorRate", DoubleValue (errorRate));

  star.GetSpokeNode (0)->GetDevice (0)->
    SetAttribute ("ReceiveErrorModel", PointerValue (errorModel1));
  star.GetSpokeNode (1)->GetDevice (0)->
    SetAttribute ("ReceiveErrorModel", PointerValue (errorModel2));
  errorModel1->Enable ();
  errorModel2->Enable ();

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
member class function. Also, we have enabled the population of the routing
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

Default run
^^^^^^^^^^^

After building the project, run the example by typing: ::

  ./build/linux/wired_broadcast/wired_broadcast

You will get an output like this: ::

  Sending a combination
  Received one packet at decoder 1
  Received one packet at decoder 2
  Sending a combination
  Received one packet at decoder 1
  Received one packet at decoder 2
  Sending a combination
  Received one packet at decoder 1
  Received one packet at decoder 2
  Sending a combination
  Received one packet at decoder 2
  Sending a combination
  Received one packet at decoder 1
  Received one packet at decoder 2
  Sending a combination
  Received one packet at decoder 1
  Received one packet at decoder 2
  Decoding completed! Total transmissions: 6

Now we can see when a packet is received at each decoder. As expected, a packet
is sent every time slot to both decoders and the process stops when both
decoders have :math:`g` l.i. combinations. We can observe this behaviour in the
previous output. At the 4th transmission, receiver 1 did not get the combination
although receiver 2 did. Nevertheless, this is compensated in the last
transmission where receiver 1 gets its remaining combination. Besides,
receiver 2 gets a non-innovative extra combination which occurs for the
packet being sent to both decoders.


Again, we can verify for a broadcast with coding scenario that on average we
need 9.4847 transmissions for :math:`q = 2, g = 5, \epsilon = 0.3` and 2 users.
To verify it, save the following script as ``mean_packets.bash``. As you will
notice, it is a modification of the script used in the first example.

.. code-block:: bash

  #!/bin/bash

  #Check the number of extra transmission per generation

  SUM=0
  N=$1  # Number of runs

  #  For-loop with range for bash
  #  Basically run the experiment several times and collect the total
  #  transmissions to get the average

  for (( c=1; c<=${N}; c++ ))
  do
      COMB=`./build/linux/wired_broadcast/wired_broadcast | \
            grep "Total transmissions:" | cut -f5 -d\ `
      SUM=$(( ${SUM} + ${COMB} ))
  done

  BROADCAST_MEAN=`echo "scale= 4; (${SUM} / ${N})" | bc`

  echo "Wired broadcast example mean: ${BROADCAST_MEAN}"

To change its settings do a ``chmod 755 extra_packet_per_generation.bash``. Run
it in a similar way as first script. You will get an output similar to this: ::

  ./mean_packets.bash 1
  Wired broadcast example mean: 9.0000
  ./mean_packets.bash 10
  Wired broadcast example mean: 12.0000
  ./mean_packets.bash 100
  Wired broadcast example mean: 10.2500
  ./mean_packets.bash 1000
  Wired broadcast example mean: 10.0200
  ./mean_packets.bash 10000
  Wired broadcast example mean: 9.5914

As we check, by increasing the numbers of runs we see that the mean number of
transmissions to decode in a pure broadcast RLNC for two receivers, converges
to 9.4847 transmissions for the previous setting. We will also set some
parameters to observe the difference in the total number of transmissions.

Changing the field size
^^^^^^^^^^^^^^^^^^^^^^^

Set ``fifi::binary8`` as the field size in the encoder and decoder templates,
rebuild your project and rerun the previous script. You will get an output
similar to this: ::

   ./mean_packets.bash 1
   Wired broadcast example mean: 8.0000
   ./mean_packets.bash 10
   Wired broadcast example mean: 6.0000
   ./mean_packets.bash 100
   Wired broadcast example mean: 8.5600
   ./mean_packets.bash 1000
   Wired broadcast example mean: 8.2630
   ./mean_packets.bash 10000
   Wired broadcast example mean: 8.2335

Now we observe that the amount of transmissions reduces to less than 9
transmissions on average. Similarly as with the WiFi example, in this case the
decoding probability increases with a higher field size for each decoder. This
ensures that, on average, each decoder requires less transmissions to complete
decoding.

Changing the packet erasure rate
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

One interesting feature that we have added is a ``RateErrorModel`` which
basically includes a packet error rate at each receiver. Currently we have set
the error rates to be both the same and a 30% loss rate is set by default. Keep
``fifi::binary8`` as a field size in order to exclude retransmissions due to
linear dependency and account them only for losses. As we saw, with 30% losses
we see an average of 8.2335 transmissions (for 10000 example runs). Now, we will
check that by adjusting the loss rate (with the same amount of runs). So, we
just need to make some small modifications of our bash script.

We will add a new input parameter to set the loss rate and call it in the script
as follows:

.. code-block:: bash

  #!/bin/bash

  #Check the number of extra transmission per generation

  SUM=0
  N=$1  # Number of runs
  LOSS_RATE=$2  # Loss rate for both links

  #  For-loop with range for bash
  #  Basically run the experiment several times and collect the total
  #  transmissions to get the average

  for (( c=1; c<=${N}; c++ ))
  do
      COMB=`./build/linux/wired_broadcast/wired_broadcast \
            --errorRate=${LOSS_RATE} | grep "Total transmissions:" | \
            cut -f5 -d\ `
      SUM=$(( ${SUM} + ${COMB} ))
  done

  BROADCAST_MEAN=`echo "scale= 4; (${SUM} / ${N})" | bc`

  echo "Wired broadcast example mean: ${BROADCAST_MEAN}"

Save the changes in the script. Then, let us observe the output with 10% and
50% losses in both links: ::

  ./mean_packets.bash 100000 0.1
  Wired broadcast example mean: 6.0049
  ./mean_packets.bash 100000 0.5
  Wired broadcast example mean: 9.0138

For the required erasures rate, we observe that if we modify the erasure rate
in the links, the expected number of transmissions changes in the respective
manner.

Review pcap traces
^^^^^^^^^^^^^^^^^^

We have added this time also a trace file per each net device in order to
observe packet routing. ::

  > tcpdump -r star-0-0.pcap -nn -tt
  reading from file star-0-0.pcap, link-type PPP (PPP)
  1.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  2.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  3.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  4.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  5.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  6.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  7.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  8.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  9.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  > tcpdump -r star-0-1.pcap -nn -tt
  reading from file star-0-1.pcap, link-type PPP (PPP)
  1.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  2.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  3.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  4.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  5.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  6.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  7.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  8.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  9.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  > tcpdump -r star-1-0.pcap -nn -tt
  reading from file star-1-0.pcap, link-type PPP (PPP)
  1.252929 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  2.252929 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  6.252929 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  8.252929 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  9.252929 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  > tcpdump -r star-2-0.pcap -nn -tt
  reading from file star-2-0.pcap, link-type PPP (PPP)
  1.252929 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  2.252929 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  6.252929 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  7.252929 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  8.252929 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006

From the trace files we see the broadcast nature set before. We have two net
devices in the transmitter given that the ``PointToPointStarHelper`` creates
a point-to-point link at each receiver. By setting ``SetAllowBroadcast (true)``
in the transmitter socket, we ensure to be using the broadcast channel on the
source node.
