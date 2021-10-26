Recoders with Erasure Channels
==============================

.. _recoders:

This example considers a transmitter sending coded packets with RLNC from a
generation size :math:`g` and field size :math:`q` to a decoder. Packets are
set through :math:`N` relays in a broadcast erasure channel. Each link from the
encoder to the relays has the same erasure rate :math:`\epsilon_{E-R}`.
In the second hop, all the links from the relays to the decoder have a
common erasure rate :math:`\epsilon_{R-D}`. By default, we consider:
:math:`g = 5`, :math:`q = 2^{8}`, :math:`N = 2`, :math:`\epsilon_{E-R} = 0.4`
and :math:`\epsilon_{R-D} = 0.2`. Topology is shown as follows:

.. literalinclude:: ../examples/kodo-recoders.cc
   :language: c++
   :start-after: //! [0]
   :end-before: //! [1]
   :linenos:

What to Simulate
----------------

* Behavior: The sender keeps transmitting the generation until the
  all the relays have :math:`g` linearly independent (l.i.) coded
  packets. The relays keep sending packets until the decoder has received
  this data. As with previous examples, packets might or might not be loss at the
  given erasure rates in the respective links. Additionally, we have 2 transmission
  policies depending if recoding is enabled or not. If recoding is enabled
  (default), received packets in each recoder are recoded to create new
  coded packets which are sent to the decoder. If recoding is disabled,
  any previously received packet is forwarded to the decoder. The packet
  to be forwarded is selected at random. To avoid collisions from the
  relays, all the relays access the medium to transmit with probability
  :math:`p`. If two or more relays access the medium at the same time,
  then event simulator schedules them to transmit in a non-colliding
  order.
* Inputs: Main parameters will be generation size, field size, number of
  relays, packet losses in each hop, a boolean flag for the recoding policy
  and a transmit probability to access the shared medium.
* Outputs: Two counters to indicate how much transmissions did
  the process required and some prints to indicate when decoding is completed.
  The number of transmissions should change as we vary the input parameters.
* Scenarios: We will variate the generation and field size to verify
  theoretical expected values regarding the amount of transmissions to
  decode.

Program Description
-------------------

In your ``~/ns-3-dev/examples/kodo`` folder, you will see the ``kodo-recoders.cc``
file which contains the source code of this simulation. Its structure is
similar to previous simulations, so again we will focus on the main
differences.

Header Includes
^^^^^^^^^^^^^^^

.. literalinclude:: ../examples/kodo-recoders.cc
   :language: c++
   :start-after: //! [2]
   :end-before: //! [3]
   :linenos:

The ``Recoders`` class in ``kodo-recoders.h`` is used to model the expected
behavior of the nodes in our simulation.

Simulation Class
^^^^^^^^^^^^^^^^

The ``Recoders`` design is similar as the one for
``Broadcast``. Still, some differences exist. For this case
``kodo::block::decoder`` is the type for the relays since it behaves
in the same way.

Also, now we add different functions for what a relay might
perform. Hereby, we include ``SendPacketRecoder`` and ``ReceivePacketRecoder``
to split the functionality of recoding (or forwarding) and receiving with
decoding. The recoding functionality is performed again with
``recoder.recode_symbol()``.

For control variables, for the recoding or forwarding behavior, we included a
boolean as a construction argument. Also we keep track of the decoder rank
with a counter, in order to notify when a coded packet is received at the
decoder. Furthermore, for each of the relays we store its received
packets so we can forward one of them at random later. 

For the medium probability access :math:`p`, we use samples from a
``ns3::Ptr<ns3::UniformRandomVariable>`` and convert them to
samples of a Bernoulli random variable at a given transmission using the
`Inverse Transmformating Sampling <https://en.wikipedia.org/wiki/Inverse_transform_sampling>`_.
In this way, we guarantee that a node attempts a medium access with the
desired probability. Finally, we include a counter for the total number of transmissions
from all the relays for counting total transmissions in general.

Default Parameters and Command-line Parsing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For the default parameters, we show what has been added for this example:

.. code-block:: c++

  // Main parameters
  double errorRateEncoderRecoder = 0.4; // Error rate for encoder-recoder link
  double errorRateRecoderDecoder = 0.2; // Error rate for recoder-decoder link
  bool recodingFlag = true; // Flag to control recoding
  uint32_t recoders = 2; // Number of recoders
  std::string field = "binary"; // Finite field used
  double transmitProbability = 0.5; // Transmit probability for the relays

  // Command parsing
  cmd.AddValue ("errorRateEncoderRecoder",
                "Packet erasure rate for the encoder-recoder link",
                errorRateEncoderRecoder);
  cmd.AddValue ("errorRateRecoderDecoder",
                "Packet erasure rate for the recoder-decoder link",
                errorRateRecoderDecoder);
  cmd.AddValue ("recodingFlag", "Enable packet recoding", recodingFlag);
  cmd.AddValue ("recoders", "Amount of recoders", recoders);
  cmd.AddValue ("field", "Finite field used", field);
  cmd.AddValue ("transmitProbability", "Transmit probability from recoder",
                transmitProbability);



Topology and Net Helpers
^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../examples/kodo-recoders.cc
   :language: c++
   :start-after: //! [4]
   :end-before: //! [5]
   :linenos:

In order to be able to construct various net devices in the relays from the
helpers, we separate the relays in 2 subnets: a one-to-many subnet which
covers the encoder and the relays, and a many-to-one subnet which covers
the relays and the decoder. For the one-to-many (broadcast) subnet, we use
the ``PointToPointStarHelper`` and for the many-to-one we create the net
devices with the ``Install`` member function of the ``PointToPointHelper``
and store it in a container for easy IP address assignment. Then, we assign
the IP addresses as shown in the previous topology figure. For the one-to-many,
we use the ``"10.1.X.X"`` subnet and for the many-to-one, the ``"10.2.1.X"``
subnet.

Simulation Event Handler
^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../examples/kodo-recoders.cc
   :language: c++
   :start-after: //! [6]
   :end-before: //! [7]
   :linenos:

Now we aggregate the generation of coded packets from the relays to the
scheduling process. We send packets from each recoder independently from
previously having received a packet. However, the recoder will only send
coded packets from the coded packets that it has l.i. packets and it
sucessfully access the medium. If some relays send their coded packet at,
the same time, the event scheduling organizes them properly.

Simulation Runs
---------------

Default Run
^^^^^^^^^^^

To run the default simulation, just type: ::

  python waf --run kodo-recoders

You will see an output similar to this: ::

  +-----------------------------------+
  |Sending a coded packet from ENCODER|
  +-----------------------------------+
  Received a packet at RECODER 2
  +-------------------------------------+
  |Sending a coded packet from RECODER 2|
  +-------------------------------------+
  Received an innovative packet at DECODER!
  Decoder rank: 1

  +-----------------------------------+
  |Sending a coded packet from ENCODER|
  +-----------------------------------+
  Received a packet at RECODER 1
  +-------------------------------------+
  |Sending a coded packet from RECODER 1|
  +-------------------------------------+
  +-------------------------------------+
  |Sending a coded packet from RECODER 2|
  +-------------------------------------+
  Received an innovative packet at DECODER!
  Decoder rank: 2

  +-----------------------------------+
  |Sending a coded packet from ENCODER|
  +-----------------------------------+
  Received a packet at RECODER 1
  +-------------------------------------+
  |Sending a coded packet from RECODER 1|
  +-------------------------------------+
  +-------------------------------------+
  |Sending a coded packet from RECODER 2|
  +-------------------------------------+
  Received an innovative packet at DECODER!
  Decoder rank: 3

  *** Decoding completed! ***
  Encoder transmissions: 3
  Recoders transmissions: 5
  Total transmissions: 8

From the simulation output, it can be seen that in the first transmission only
recoder 2 got the coded packet from the source and it conveyed properly to
the decoder. Here recoder 1 does not make any transmissions since it does
not have any possible information to convey. For the second and third
transmission, recoder 1 got the packet and conveyed properly
to the decoder. Observe that for the second and third transmissions, recoder
2 did not get any coded packets but it still tries to send them. However,
it will only be possible to send only one degree of freedom given that its set
of packets only allow this. Whenever it receives more combinations, it will be
possible for it to send more. You can modify the number of relays and erasure
rates in the hops to check the effects in the number of transmissions. Also,
you may verify the pcap traces as well. We invite you to modify the parameters
as you might prefer to verify your intuitions and known results.
