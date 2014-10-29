// This object implements both the sender (encoder),
// forwarders (recoders), decoder and other properties of the simulation

#include <kodo/rlnc/full_rlnc_codes.hpp>
#include <kodo/trace.hpp>
#include <kodo/wrap_copy_payload_decoder.hpp>

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

  EncoderRecodersDecoderRlnc(
    const uint32_t users,
    const uint32_t generationSize,
    const uint32_t packetSize,
    const std::vector<ns3::Ptr<ns3::Socket>>& recodersSockets,
    const bool recodingFlag)
    : m_users(users),
      m_generationSize(generationSize),
      m_packetSize(packetSize),
      m_recodersSockets(recodersSockets),
      m_recodingFlag(recodingFlag)
  {

    // Call factories from basic parameters
    typename rlnc_encoder::factory encoder_factory (m_generationSize,
                                                    m_packetSize);
    typename rlnc_recoder::factory recoder_factory (m_generationSize,
                                                    m_packetSize);
    typename rlnc_decoder::factory decoder_factory (m_generationSize,
                                                    m_packetSize);

    // Encoder creation and settings
    m_encoder = encoder_factory.build ();
    m_encoder->set_systematic_off ();
    m_encoder->seed (time (0));

    // Initialize the input data
    std::vector<uint8_t> data (m_encoder->block_size (), 'x');
    m_encoder->set_symbols (sak::storage(data));
    m_payload_buffer.resize (m_encoder->payload_size ());

    // Recoders creation and settings
    m_recoders = std::vector<recoder_pointer> (m_users);

    for (uint32_t n = 0; n < m_users; n++)
     {
       m_recoders[n] = recoder_factory.build();
       m_socketMap[m_recodersSockets[n]] = m_recoders[n];
     }

    // Encoder creation and settings
    m_decoder = decoder_factory.build ();

    // Initialize transmission counts
    m_encoder_transmission_count = 0;
    m_recoders_transmission_count = 0;
    m_decoder_rank = 0;
  }

  void SendPacketEncoder (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
  {
    bool all_recoders_decoded = true;

    for(auto recoder : m_recoders)
      {
         all_recoders_decoded = all_recoders_decoded && recoder->is_complete();
      }

    if (!all_recoders_decoded && (m_recodingFlag && !m_decoder->is_complete()))
      {
        std::cout << "+----------------------------------+"   << std::endl;
        std::cout << "|Sending a combination from ENCODER|"   << std::endl;
        std::cout << "+----------------------------------+\n" << std::endl;

        uint32_t bytes_used = m_encoder->encode(&m_payload_buffer[0]);
        auto packet = ns3::Create<ns3::Packet> (&m_payload_buffer[0],
                                                bytes_used);
        socket->Send (packet);
        m_encoder_transmission_count++;

        if (kodo::has_trace<rlnc_encoder>::value)
          {
            std::cout << "Trace encoder:" << std::endl;
            kodo::trace(m_encoder, std::cout);
          }

        ns3::Simulator::Schedule (
          pktInterval,
          &EncoderRecodersDecoderRlnc<field,
                                      encoderTrace,
                                      decoderTrace>::SendPacketEncoder,
          this,
          socket,
          pktInterval);
      }
    else
      {
        socket->Close ();
      }
  }

  void ReceivePacketRecoder (ns3::Ptr<ns3::Socket> socket)
  {
    recoder_pointer recoder = m_socketMap[socket];
    auto packet = socket->Recv();
    packet->CopyData(&m_payload_buffer[0], recoder->payload_size());

    if (!m_recodingFlag)
      {
        m_previous_packet = packet;
      }

    auto id = std::distance(std::begin(m_socketMap),
                            m_socketMap.find(socket)) + 1;

    recoder->decode(&m_payload_buffer[0]);
    std::cout << "Received a coded packet at RECODER " << id << std::endl;

    if (kodo::has_trace<rlnc_recoder>::value)
      {
        auto filter = [](const std::string& zone)
        {
          std::set<std::string> filters =
            {"decoder_state","input_symbol_coefficients"};
          return filters.count(zone);
        };

        std::cout << "Trace recoder " << id << ": " << std::endl;
        kodo::trace(recoder, std::cout, filter);
      }
    if (recoder->is_complete())
      {
        std::cout << "** Recoder " << id << " is full rank! **\n" << std::endl;
      }
  }

  void SendPacketRecoder (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
  {
    recoder_pointer recoder = m_socketMap[socket];
    auto id = std::distance(std::begin(m_socketMap),
                            m_socketMap.find(socket)) + 1;

    if (!m_decoder->is_complete())
      {
        if (m_recodingFlag)
          {
            std::cout << "+------------------------------------+"
                      << std::endl;
            std::cout << "|Sending a combination from RECODER "
                      << id << "|"   << std::endl;
            std::cout << "+------------------------------------+\n"
                      << std::endl;

            // Recode a new packet and send
            uint32_t bytes_used = recoder->recode(&m_payload_buffer[0]);
            auto packet = ns3::Create<ns3::Packet> (&m_payload_buffer[0],
                                                    bytes_used);
            socket->Send (packet);
            m_recoders_transmission_count++;
          }
        else
          {
            if(!recoder->rank ())
              {
                std::cout << "No packet from ENCODER to forward!" << std::endl;
              }
            else
              {
                auto packet = m_previous_packet;

                // Remove all packet tags in order to the callback retag them to avoid
                // ~/ns-3-dev/src/common/packet-tag-list.cc, line=139 assert failure.
                // Tag removal is shown in ~/ns-3-dev/src/applications/udp-echo/
                // udp-echo-server.cc for packet forwarding

                packet->RemoveAllPacketTags ();
                socket->Send (packet);
                m_recoders_transmission_count++;
                std::cout << "Forwarding a previous packet from RECODER...\n"
                          << std::endl;
              }
         }

        ns3::Simulator::Schedule (
          pktInterval,
          &EncoderRecodersDecoderRlnc<field,
                                      encoderTrace,
                                      decoderTrace>::SendPacketRecoder,
          this,
          socket,
          pktInterval);
      }
    else
      {
        socket->Close ();
        std::cout << "*** Decoding completed! ***" << std::endl;
        std::cout << "Encoder transmissions: " << m_encoder_transmission_count
                  << std::endl;
        std::cout << "Recoders transmissions: " << m_recoders_transmission_count
                  << std::endl;
        std::cout << "Total transmissions: "
                  << m_encoder_transmission_count + \
                     m_recoders_transmission_count << std::endl;
      }
  }

  void ReceivePacketDecoder (ns3::Ptr<ns3::Socket> socket)
  {
    auto packet = socket->Recv();
    packet->CopyData(&m_payload_buffer[0], m_decoder->payload_size());
    m_decoder->decode(&m_payload_buffer[0]);

    if (m_decoder_rank != m_decoder->rank ())
      {
        std::cout << "Received a l.i. packet at DECODER!! (I)\n" << std::endl;
        std::cout << "Decoder rank is: " << m_decoder->rank() << "\n"
                  << std::endl;
        m_decoder_rank++;
      }
    else
      {
        std::cout << "Received a l.d. packet at DECODER!! (D)\n" << std::endl;
      }

    if (kodo::has_trace<rlnc_decoder>::value)
      {
        auto filter = [](const std::string& zone)
        {
          std::set<std::string> filters =
            {"decoder_state","input_symbol_coefficients"};
          return filters.count(zone);
        };

        std::cout << "Trace decoder:" << std::endl;
        kodo::trace(m_decoder, std::cout, filter);
      }
  }

private:

  //rlnc_decoder::factory::pointer m_decoder;

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