Broadcast RLNC with a N-user erasure channel
============================================

.. _wired_broadcast:

This example is similar to the first, but now the channel will
be modeled with an erasure rate. In this way, the topology considered describes
a transmitter sending coded packets with RLNC from a generation size :math:`g`,
field size :math:`q` in a broadcast erasure channel to N receivers. As with the
previous example, we will start with :math:`g = 5`, :math:`q = 2`,
:math:`N = 2` and we will again observe completion time in terms of
transmissions. Although, now we include the erasure rate (in percentage),
:math:`0 \leq \epsilon < 1`, to indicate packet losses. For this case, we
will assume that all links have the same erasure rate for simplicity,
:math:`\epsilon = 0.3`, i.e. 30% packet losses. Topology is shown as follows:

.. literalinclude:: ../src/wired_broadcast/main.cc
   :language: c++
   :start-after: //! [0]
   :end-before: //! [1]
   :linenos:

What to simulate?
-----------------

* Behaviour: The sender keeps transmitting the generation until all
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

In your local repository, you should have a folder named
``src/wired_broadcast/``. If you check it, you will see the ``main.cc`` file
which contains the source code of this simulation. Its structure is similar
to the previous one, so now we will focus on the main differences.

Default parameters and command parsing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For the default parameters, we show what has been added for the erasure rate:

.. code-block:: c++

  // Main parameters
  double errorRate = 0.3; // Error rate for all the links

  // Command parsing
  cmd.AddValue ("errorRate", "Packet erasure rate for the links", errorRate);

Topology and net helpers
^^^^^^^^^^^^^^^^^^^^^^^^

For this part, there are some changes because we have removed the WiFi protocol
and we have represented our channel as a packet erasure channel. For creating
the topology, we proceed in a different way than the used for the first
example. We use the ``PointToPointHelper`` to create a point-to-point
link type. Then, we create the links from the source to each receiver using the
``PointToPointStarHelper`` which takes as an input the desired number of links
and a ``PointToPointHelper`` instance, namely ``pointToPoint`` in our case.
After that, we create the error rate model for each net device in the topology,
configure and set it to affect the packets and enable them. Finally, we set up
the Internet stack and IP addresses to our topology.

.. literalinclude:: ../src/wired_broadcast/main.cc
   :language: c++
   :start-after: //! [2]
   :end-before: //! [3]
   :linenos:

The remaining elements of the simulation are very similar to the first example,
you can review them and check the differences.

Simulations runs
----------------

Default run
^^^^^^^^^^^

Given that now we have an erasure rate different from zero and we can control
it, packet reception will differ randomly. Then, as a default we disable
the decoder tracing and keep the reception prints in order to check just
when a packet has arrived to each receiver. After building the project, run
the example by typing: ::

  ./build/linux/src/wired_broadcast/wired_broadcast

You will get an output like this: ::

  +---------------------+
  |Sending a combination|
  +---------------------+
  Received a packet at decoder 1
  Received a packet at decoder 2
  +---------------------+
  |Sending a combination|
  +---------------------+
  Received a packet at decoder 1
  Received a packet at decoder 2
  +---------------------+
  |Sending a combination|
  +---------------------+
  Received a packet at decoder 1
  Received a packet at decoder 2
  +---------------------+
  |Sending a combination|
  +---------------------+
  Received a packet at decoder 2
  +---------------------+
  |Sending a combination|
  +---------------------+
  Received a packet at decoder 1
  Received a packet at decoder 2
  +---------------------+
  |Sending a combination|
  +---------------------+
  Received a packet at decoder 1
  Received a packet at decoder 2
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
in the links, the expected number of transmissions changes. By increasing
the error rate, we need more transmissions to overcome the losses.

Review pcap traces
^^^^^^^^^^^^^^^^^^

We have added also a trace file per each net device in order to
observe packet routing. ::

  > tcpdump -r wired-broadcast-0-0.pcap -nn -tt
  reading from file wired-broadcast-0-0.pcap, link-type PPP (PPP)
  1.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  2.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  3.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  4.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  5.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  6.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  7.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  8.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  9.000000 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  > tcpdump -r wired-broadcast-0-1.pcap -nn -tt
  reading from file wired-broadcast-0-1.pcap, link-type PPP (PPP)
  1.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  2.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  3.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  4.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  5.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  6.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  7.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  8.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  9.000000 IP 10.1.2.1.49153 > 10.1.2.255.80: UDP, length 1006
  > tcpdump -r wired-broadcast-1-0.pcap -nn -tt
  reading from file wired-broadcast-1-0.pcap, link-type PPP (PPP)
  1.252929 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  2.252929 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  6.252929 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  8.252929 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  9.252929 IP 10.1.1.1.49153 > 10.1.1.255.80: UDP, length 1006
  > tcpdump -r wired-broadcast-2-0.pcap -nn -tt
  reading from file wired-broadcast-2-0.pcap, link-type PPP (PPP)
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
