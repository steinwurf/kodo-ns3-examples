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

A main difference with the previous simulation is that now we define packet
reception function for each receiver. In the source code, you will notice that
we have 2 instances of the decoder and a new stopping condition for the
transmitter, e.g. to verify that both receivers are full rank. Otherwise,
this part is the same as the first example.

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
and we have represented our channel as a packet erasure channel.

.. code-block:: c++

  // Describe packet loss model

Internet and application protocol helpers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: c++

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

Simulation runs
---------------

Default run
^^^^^^^^^^^
