Encoder, Recoders, Decoder with Erasure Channels
================================================

.. _encoder_recoder_decoder:

This example considers a transmitter sending coded packets with RLNC from a
generation size :math:`g` and field size :math:`q` to a decoder. Packets are
set through N recoders in a broadcast erasure channel. Each link from the
encoder to the recoders has the same erasure rate :math:`\epsilon_{E-R}`.
In the second hop, all the links from the recoders to the decoder have a
common erasure rate :math:`\epsilon_{R-D}`. By default, we consider:
:math:`g = 5`, :math:`q = 2^{8}`, :math:`N = 2`, :math:`\epsilon_{E-R} = 0.4`
and :math:`\epsilon_{R-D} = 0.2`. Topology is shown as follows:

.. literalinclude:: ../src/encoder_recoder_decoder/main.cc
   :language: c++
   :start-after: //! [0]
   :end-before: //! [1]
   :linenos:

What to Simulate
----------------

* Behavior: The sender keeps transmitting the generation until the
  decoder or all the recoders have :math:`g` linearly independent (l.i.) coded
  packets. As with previous examples, packets might or might not be loss at the
  given rates in the respective links. Additionally, we have 2 operation
  policies depending if recoding is enabled or not. If recoding is enabled
  (default), received packets in each recoder are recoded to create new
  coded packets which are sent to the decoder. If recoding is disabled,
  only the last received packet is forwarded to the decoder.
* Inputs: Main parameters will be generation size, field size, number of
  recoders and packet losses in each hop.
* Outputs: Two counters to indicate how much transmissions did
  the process required and some prints to indicate when decoding is completed.
  The number of transmissions should change as we vary the input parameters.
* Scenarios: We will variate the generation and field size to verify
  theoretical expected values regarding the amount of transmissions to
  decode.

Program Description
-------------------

In your local repository, you should have a folder named
``src/encoder_recoder_decoder/``. If you check it, you will see the ``main.cc``
file which contains the source code of this simulation. Its structure is similar
to previous simulations, so now we will focus on the main differences.

Header Includes
^^^^^^^^^^^^^^^

.. literalinclude:: ../src/encoder_recoder_decoder/main.cc
   :language: c++
   :start-after: //! [2]
   :end-before: //! [3]
   :linenos:

The ``EncoderRecodersDecoderRlnc`` class in ``encoder-recoders-decoder.h``
is used to model the expected behavior of the nodes in our simulation.

Simulation Class
^^^^^^^^^^^^^^^^

The ``EncoderRecodersDecoderRlnc`` class can be roughly defined in the
following way:

.. code-block:: c++

  template<class field, class encoderTrace, class decoderTrace>
  class EncoderRecodersDecoderRlnc
  {
  public:

    using rlnc_encoder = typename kodo::full_rlnc_encoder<field, encoderTrace>;
    using rlnc_decoder = typename kodo::full_rlnc_decoder<field, decoderTrace>;

    using rlnc_recoder = typename kodo::wrap_copy_payload_decoder<rlnc_decoder>;

    using encoder_pointer = typename rlnc_encoder::factory::pointer;
    using recoder_pointer = typename rlnc_recoder::factory::pointer;
    using decoder_pointer = typename rlnc_decoder::factory::pointer;

    EncoderRecodersDecoderRlnc (
      const uint32_t users,
      const uint32_t generationSize,
      const uint32_t packetSize,
      const std::vector<ns3::Ptr<ns3::Socket>>& recodersSockets,
      const bool recodingFlag)
      : m_users (users),
        m_generationSize (generationSize),
        m_packetSize (packetSize),
        m_recodersSockets (recodersSockets),
        m_recodingFlag (recodingFlag)
    {
      // Constructor
    }

    void SendPacketEncoder (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
    {
      // Encoder logic
    }

    void ReceivePacketRecoder (ns3::Ptr<ns3::Socket> socket)
    {
      // Recoders logic for reception
    }

    void SendPacketRecoder (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
    {
      // Recoders logic for transmission
    }

    void ReceivePacketDecoder (ns3::Ptr<ns3::Socket> socket)
    {
      // Decoder logic
    }

  private:

    const uint32_t m_users;
    const uint32_t m_generationSize;
    const uint32_t m_packetSize;
    const bool m_recodingFlag;

    encoder_pointer m_encoder;
    std::vector<recoder_pointer> m_recoders;
    decoder_pointer m_decoder;
    std::vector<ns3::Ptr<ns3::Socket>> m_recodersSockets;
    std::map<ns3::Ptr<ns3::Socket>,recoder_pointer> m_socketMap;

    std::vector<uint8_t> m_payload_buffer;
    uint32_t m_encoder_transmission_count;
    uint32_t m_recoders_transmission_count;
    uint32_t m_decoder_rank;
    ns3::Ptr<ns3::Packet> m_previous_packet;
  };

The ``EncoderRecodersDecoderRlnc`` design is similar as the one for
``BroadcastRlnc``. Still, some differences exist. For this case
``rlnc_decoder`` is the default type since we just have one decoder. However,
for the recoders we will have many and then, we need the copy payload API.

Also, now we add different functions for the functionalities that a recoder can
perform. Hereby, we include ``SendPacketRecoder`` and ``ReceivePacketRecoder``
to split the functionality of recoding (or forwarding) and receiving with
decoding. The recoding functionality is performed with ``recoder->recode ()``,
where ``recoder`` is a pointer to a ``rlnc_recoder``.

For control variables, for the recoding or forwarding behavior, we included a
boolean as a construction argument. Also we keep track of the decoder rank
with a counter, in order to notify when a l.i. combination is received at the
decoder. Finally, we include a counter for the total number of transmissions
from all the recoders for counting total transmissions in general.

Default Parameters and Command-line Parsing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For the default parameters, we show what has been added for this example:

.. code-block:: c++

  // Main parameters
  double errorRateEncoderRecoder = 0.4; // Error rate for encoder-recoder link
  double errorRateRecoderDecoder = 0.2; // Error rate for recoder-decoder link
  bool recodingFlag = true; // Flag to control recoding
  uint32_t recoders = 2; // Number of recoders

  // Command parsing
  cmd.AddValue ("errorRateEncoderRecoder",
                "Packet erasure rate for the encoder-recoder link",
                errorRateEncoderRecoder);
  cmd.AddValue ("errorRateRecoderDecoder",
                "Packet erasure rate for the recoder-decoder link",
                errorRateRecoderDecoder);
  cmd.AddValue ("recodingFlag", "Enable packet recoding", recodingFlag);
  cmd.AddValue ("recoders", "Amount of recoders", recoders);


Topology and Net Helpers
^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../src/encoder_recoder_decoder/main.cc
   :language: c++
   :start-after: //! [4]
   :end-before: //! [5]
   :linenos:

In order to be able to construct various net devices in the recoders from the
helpers, we separate the recoders in 2 subnets: a one-to-many subnet which
covers the encoder and the recoders, and a many-to-one subnet which covers
the recoders and the decoder. For the one-to-many (broadcast) subnet, we use
the ``PointToPointStarHelper`` and for the many-to-one we create the net
devices with the ``Install`` member function of the ``PointToPointHelper``
and store it in a container for easy IP address assignment. Then, we assign
the IP addresses as shown in the previous topology figure. For the one-to-many,
we use the ``"10.1.X.X"`` subnet and for the many-to-one, the ``"10.2.1.X"``
subnet.

Simulation Event Handler
^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../src/encoder_recoder_decoder/main.cc
   :language: c++
   :start-after: //! [6]
   :end-before: //! [7]
   :linenos:

Now we aggregate the generation of coded packets from the recoders to the
scheduling process. We send packets from each recoder independently from
previously having received a packet. However, the recoder will only send
combinations from the coded packets that it has. All recoders send their
coded packet at the same time.

Simulation Runs
---------------

Default Run
^^^^^^^^^^^

To run the default simulation, just type: ::

  ./build/linux/src/encoder_recoder_decoder/encoder_recoder_decoder

You will see an output similar to this: ::

  +----------------------------------+
  |Sending a combination from ENCODER|
  +----------------------------------+
  Received a coded packet at RECODER 2

  +------------------------------------+
  |Sending a combination from RECODER 1|
  +------------------------------------+

  +------------------------------------+
  |Sending a combination from RECODER 2|
  +------------------------------------+

  Received a l.i. packet at DECODER! (I)
  Decoder rank: 1

  +----------------------------------+
  |Sending a combination from ENCODER|
  +----------------------------------+
  Received a coded packet at RECODER 1

  +------------------------------------+
  |Sending a combination from RECODER 1|
  +------------------------------------+

  +------------------------------------+
  |Sending a combination from RECODER 2|
  +------------------------------------+

  Received a l.i. packet at DECODER! (I)
  Decoder rank: 2

  +----------------------------------+
  |Sending a combination from ENCODER|
  +----------------------------------+
  Received a coded packet at RECODER 1

  +------------------------------------+
  |Sending a combination from RECODER 1|
  +------------------------------------+

  +------------------------------------+
  |Sending a combination from RECODER 2|
  +------------------------------------+

  Received a l.i. packet at DECODER! (I)
  Decoder rank: 3

  *** Decoding completed! ***
  Encoder transmissions: 3
  Recoders transmissions: 6
  Total transmissions: 9
  *** Decoding completed! ***
  Encoder transmissions: 3
  Recoders transmissions: 6
  Total transmissions: 9

From the simulation output, it can be seen that in the first transmission only
recoder 2 got the coded packet from the source and it conveyed properly to
the decoder. For the second and third transmission, recoder 1 got the packet
and conveyed properly to the decoder. You can modify the number of recoders
and erasure rates in the hops to check the effects in the number of
transmissions.