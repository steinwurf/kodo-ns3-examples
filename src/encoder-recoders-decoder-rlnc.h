/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Steinwurf ApS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// This object implements RLNC (random linear network coding) in
// the application layer for a encoder - N recoders - decoders topology.

#pragma once

#include <kodocpp/kodocpp.hpp>

class EncoderRecodersDecoderRlnc
{
public:

  EncoderRecodersDecoderRlnc (const kodo_code_type codeType,
    const kodo_finite_field field, const bool enableTrace,
    const uint32_t users, const uint32_t generationSize,
    const uint32_t packetSize,
    const std::vector<ns3::Ptr<ns3::Socket>>& recodersSockets,
    const bool recodingFlag)
    : m_codeType (codeType),
      m_field (field),
      m_enableTrace (enableTrace),
      m_users (users),
      m_generationSize (generationSize),
      m_packetSize (packetSize),
      m_recodingFlag (recodingFlag),
      m_recodersSockets (recodersSockets)
  {
    srand(static_cast<uint32_t>(time(0)));

    // Call factories from basic parameters

    kodocpp::encoder_factory encoder_factory (m_codeType, m_field,
      m_generationSize, m_packetSize, m_enableTrace);
    kodocpp::decoder_factory recoder_factory (m_codeType, m_field,
      m_generationSize, m_packetSize, m_enableTrace);
    kodocpp::decoder_factory decoder_factory (m_codeType, m_field,
      m_generationSize, m_packetSize, m_enableTrace);

    // Create encoder and disable systematic mode
    m_encoder = encoder_factory.build ();
    m_encoder.set_systematic_off ();

    // Initialize the input data
    std::vector<uint8_t> data_in (m_encoder.block_size (), 'x');
    m_encoder.set_symbols (data_in.data (), m_encoder.block_size ());
    m_payload.resize (m_encoder.payload_size ());

    // Create recoders and place them in a vector
    for (uint32_t n = 0; n < m_users; n++)
     {
       m_recoders.emplace_back (recoder_factory.build ());
     }

    // Encoder creation and settings
    m_decoder = decoder_factory.build ();

    // Initialize transmission counts
    m_encoder_transmission_count = 0;
    m_recoders_transmission_count = 0;
    m_decoder_rank = 0;

    m_previous_packets = std::vector<ns3::Ptr<ns3::Packet>> (m_users);
  }

  void SendPacketEncoder (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
  {
    bool all_recoders_decoded = true;

    for (auto recoder : m_recoders)
      {
         all_recoders_decoded &= recoder.is_complete ();
      }

    if (!(m_decoder.is_complete () || (m_recodingFlag && all_recoders_decoded)))
      {
        std::cout << "+-----------------------------------+" << std::endl;
        std::cout << "|Sending a coded packet from ENCODER|" << std::endl;
        std::cout << "+-----------------------------------+" << std::endl;

        uint32_t bytes_used = m_encoder.write_payload (&m_payload[0]);
        auto packet = ns3::Create<ns3::Packet>  (&m_payload[0], bytes_used);
        socket->Send (packet);
        m_encoder_transmission_count++;

        ns3::Simulator::Schedule (pktInterval,
          &EncoderRecodersDecoderRlnc::SendPacketEncoder, this,
          socket, pktInterval);
      }
    else
      {
        socket->Close ();
      }
  }

  void ReceivePacketRecoder (ns3::Ptr<ns3::Socket> socket)
  {
    // Find the recoder index based on the socket
    auto it =
      std::find(m_recodersSockets.begin (), m_recodersSockets.end (), socket);
    auto id = std::distance (m_recodersSockets.begin (), it);

    std::cout << "Received a packet at RECODER " << id + 1 << std::endl;

    kodocpp::decoder recoder = m_recoders[id];

    auto packet = socket->Recv ();
    packet->CopyData (&m_payload[0], recoder.payload_size ());

    recoder.read_payload (&m_payload[0]);

    if (!m_recodingFlag)
      {
        m_previous_packets[id] = packet;
      }

    if (recoder.is_complete ())
      {
        std::cout << "RECODER " << id + 1 << " is complete!\n" << std::endl;
      }
  }

  void SendPacketRecoder (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
  {
    // Find the recoder index based on the socket
    auto it =
      std::find(m_recodersSockets.begin (), m_recodersSockets.end (), socket);
    auto id = std::distance (m_recodersSockets.begin (), it);

    kodocpp::decoder recoder = m_recoders[id];

    if (!m_decoder.is_complete () && recoder.rank () > 0)
      {
        if (m_recodingFlag)
          {
            std::cout << "+-------------------------------------+" << std::endl;
            std::cout << "|Sending a coded packet from RECODER "  << id + 1
                      << "|" << std::endl;
            std::cout << "+-------------------------------------+" << std::endl;

            // Recode a new packet and send
            uint32_t bytes_used = recoder.write_payload (&m_payload[0]);
            auto packet = ns3::Create<ns3::Packet> (&m_payload[0], bytes_used);
            socket->Send (packet);
            m_recoders_transmission_count++;
          }
        else
          {
            auto packet = m_previous_packets[id];

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

        ns3::Simulator::Schedule (pktInterval,
          &EncoderRecodersDecoderRlnc::SendPacketRecoder, this,
          socket, pktInterval);
      }
  }

  void ReceivePacketDecoder (ns3::Ptr<ns3::Socket> socket)
  {
    auto packet = socket->Recv ();
    packet->CopyData (&m_payload[0], m_decoder.payload_size ());
    m_decoder.read_payload (&m_payload[0]);

    if (m_decoder.rank () > m_decoder_rank)
      {
        std::cout << "Received an innovative packet at DECODER!" << std::endl;
        std::cout << "Decoder rank: " << m_decoder.rank () << "\n"
          << std::endl;
        m_decoder_rank = m_decoder.rank ();

        if (m_decoder.is_complete ())
          {
            socket->Close ();
            std::cout << "*** Decoding completed! ***" << std::endl;
            std::cout << "Encoder transmissions: "
                      << m_encoder_transmission_count << std::endl;
            std::cout << "Recoders transmissions: "
                      << m_recoders_transmission_count << std::endl;
            uint32_t total = m_encoder_transmission_count +
                             m_recoders_transmission_count;
            std::cout << "Total transmissions: " << total << std::endl;
          }
      }
  }

private:

  const kodo_code_type m_codeType;
  const kodo_finite_field m_field;
  const bool m_enableTrace;
  const uint32_t m_users;
  const uint32_t m_generationSize;
  const uint32_t m_packetSize;
  const bool m_recodingFlag;

  kodocpp::encoder m_encoder;
  std::vector<kodocpp::decoder> m_recoders;
  kodocpp::decoder m_decoder;
  std::vector<ns3::Ptr<ns3::Socket>> m_recodersSockets;

  std::vector<uint8_t> m_payload;
  uint32_t m_encoder_transmission_count;
  uint32_t m_recoders_transmission_count;
  uint32_t m_decoder_rank;
  std::vector<ns3::Ptr<ns3::Packet>> m_previous_packets;
};