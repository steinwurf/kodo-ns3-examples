// Just for illustration purposes, this simple objects implements both
// the sender (encoder) and receiver (decoder).
class KodoSimulation
{
public:

  // We implement the Kodo traces (available since V.17.0.0). Here, we have
  // enabled the decoder trace and disabled the encoder trace.
  typedef kodo::full_rlnc_encoder<fifi::binary8,
                                  kodo::disable_trace> rlnc_encoder;
  typedef kodo::full_rlnc_decoder<fifi::binary8,
                                  kodo::disable_trace> rlnc_decoder;

  KodoSimulation(const rlnc_encoder::pointer& encoder,
                 const rlnc_decoder::pointer& decoder1,
                 const rlnc_decoder::pointer& decoder2)
    : m_encoder(encoder),
      m_decoder_1(decoder1),
      m_decoder_2(decoder2)
  {
    m_encoder->set_systematic_off();
    m_encoder->seed(time(0));

    // Initialize the input data
    std::vector<uint8_t> data(encoder->block_size(), 'x');
    m_encoder->set_symbols(sak::storage(data));

    m_payload_buffer.resize(m_encoder->payload_size());
    m_transmission_count = 0;
  }

  void ReceivePacket1 (ns3::Ptr<ns3::Socket> socket)
  {
    auto packet = socket->Recv();
    packet->CopyData(&m_payload_buffer[0], m_decoder_1->payload_size());
    m_decoder_1->decode(&m_payload_buffer[0]);
    std::cout << "Received one packet at decoder 1" << std::endl;

    if (kodo::has_trace<rlnc_decoder>::value)
      {
        auto filter = [](const std::string& zone)
        {
          std::set<std::string> filters =
            {"decoder_state","input_symbol_coefficients"};
          return filters.count(zone);
        };

        std::cout << "Trace decoder 1:" << std::endl;
        kodo::trace(m_decoder_1, std::cout, filter);
      }
  }

  void ReceivePacket2 (ns3::Ptr<ns3::Socket> socket)
  {
    auto packet = socket->Recv();
    packet->CopyData(&m_payload_buffer[0], m_decoder_2->payload_size());
    m_decoder_2->decode(&m_payload_buffer[0]);
    std::cout << "Received one packet at decoder 2" << std::endl;

    if (kodo::has_trace<rlnc_decoder>::value)
      {
        auto filter = [](const std::string& zone)
        {
          std::set<std::string> filters =
            {"decoder_state","input_symbol_coefficients"};
          return filters.count(zone);
        };

        std::cout << "Trace decoder 2:" << std::endl;
        kodo::trace(m_decoder_2, std::cout, filter);
      }
  }

  void GenerateTraffic (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
  {
    if (!m_decoder_1->is_complete() || !m_decoder_2->is_complete())
      {
        std::cout << "Sending a combination" << std::endl;
        uint32_t bytes_used = m_encoder->encode(&m_payload_buffer[0]);
        auto packet = ns3::Create<ns3::Packet> (&m_payload_buffer[0],
                                                bytes_used);
        socket->Send (packet);
        m_transmission_count++;

        if (kodo::has_trace<rlnc_encoder>::value)
          {
            std::cout << "Trace encoder:" << std::endl;
            kodo::trace(m_encoder, std::cout);
          }

        ns3::Simulator::Schedule (pktInterval, &KodoSimulation::GenerateTraffic,
                                  this, socket, pktInterval);
      }
    else
      {
        std::cout << "Decoding completed! Total transmissions: "
                  << m_transmission_count << std::endl;
        socket->Close ();
      }
  }

private:

  rlnc_encoder::pointer m_encoder;
  rlnc_decoder::pointer m_decoder_1;
  rlnc_decoder::pointer m_decoder_2;

  std::vector<uint8_t> m_payload_buffer;
  uint32_t m_transmission_count;
};
