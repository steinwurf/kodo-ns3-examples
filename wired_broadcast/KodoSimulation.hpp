
// Just for illustration purposes, this simple objects implements both
// the sender (encoder) and receivers (decoders).
template<class Field, class Trace>
class KodoSimulation
{
public:

  typedef typename kodo::full_rlnc_encoder<Field,Trace>::factory rlnc_encoder;
  typedef typename kodo::full_rlnc_decoder<Field,Trace>::factory rlnc_decoder;

  typedef typename rlnc_encoder::pointer encoder_pointer;
  typedef typename rlnc_decoder::pointer decoder_pointer;

  KodoSimulation(const uint32_t users,
                 const uint32_t generationSize,
                 const uint32_t packetSize)
    : m_users (users),
      m_generationSize (generationSize),
      m_packetSize (packetSize)
  {

    // Call factories from basic parameters
    rlnc_encoder encoder_factory (generationSize, packetSize);
    rlnc_decoder decoder_factory (generationSize, packetSize);

    // Encoder creation and settings
    m_encoder = encoder_factory.build ();
    m_encoder->set_systematic_off ();
    m_encoder->seed (time (0));

    // Decoders creation and settings
    m_decoders = std::vector<decoder_pointer> (users, decoder_factory.build());

    // Initialize the input data
    std::vector<uint8_t> data(m_encoder->block_size(), 'x');
    m_encoder->set_symbols(sak::storage(data));

    m_payload_buffer.resize(m_encoder->payload_size());
    m_transmission_count = 0;
  }

  void ReceivePacket (ns3::Ptr<ns3::Socket> socket)
  {
    auto packet = socket->Recv();
    // Find the associated decoder with the incoming socket
    uint32_t id = std::find(m_sockets.begin(),m_sockets.end(),socket) -
                  m_sockets.begin();
    packet->CopyData(&m_payload_buffer[0], m_decoders[id]->payload_size());
    m_decoders[id]->decode(&m_payload_buffer[0]);
    std::cout << "Received one packet at decoder " << id + 1 << std::endl;

    //if (kodo::has_trace<rlnc_decoder>::value)
    //  {
        auto filter = [](const std::string& zone)
        {
          std::set<std::string> filters =
            {"decoder_state","input_symbol_coefficients"};
          return filters.count(zone);
        };

        std::cout << "Trace decoder " << id + 1 << ": " << std::endl;
        kodo::trace(m_decoders[id], std::cout, filter);
    //  }
  }

  void GenerateTraffic (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
  {
    bool all_decoded = true;

    for (const auto decoder : m_decoders)
      {
        all_decoded = all_decoded && decoder->is_complete();
      }

    if (!all_decoded)
      {
        std::cout << "Sending a combination" << std::endl;
        uint32_t bytes_used = m_encoder->encode(&m_payload_buffer[0]);
        auto packet = ns3::Create<ns3::Packet> (&m_payload_buffer[0],
                                                bytes_used);
        socket->Send (packet);
        m_transmission_count++;

        /*if (kodo::has_trace<rlnc_encoder>::value)
          {
            std::cout << "Trace encoder:" << std::endl;
            kodo::trace(m_encoder, std::cout);
          }
        */
        ns3::Simulator::Schedule (pktInterval, &KodoSimulation::GenerateTraffic,
                                  this, socket, pktInterval);
      }
    else
      {
        std::cout << "Decoding completed! Total transmissions: "
                  << m_transmission_count << std::endl;
        socket->Close ();
        for (uint32_t n = 0; n < m_decoders.size(); n++)
          {
            std::cout << "Decoding completed for n = " << n << "? "
                      << m_decoders[n]->is_complete() << std::endl;
          }

      }
  }

private:

  uint32_t m_users;
  uint32_t m_generationSize;
  uint32_t m_packetSize;

  encoder_pointer m_encoder;
  std::vector<decoder_pointer> m_decoders;
  std::vector<ns3::Ptr<ns3::Socket>> m_sockets;

  std::vector<uint8_t> m_payload_buffer;
  uint32_t m_transmission_count;
};
