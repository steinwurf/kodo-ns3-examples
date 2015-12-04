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

// This object implements network coding in the application layer for
// a encoder - N recoders - decoders topology.

#pragma once

#include <kodocpp/kodocpp.hpp>

class EncoderRecodersDecoder
{
public:

  EncoderRecodersDecoder (const kodo_code_type codeType,
    const kodo_finite_field field, const uint32_t users,
    const uint32_t generationSize, const uint32_t packetSize,
    const std::vector<ns3::Ptr<ns3::Socket>>& recodersSockets,
    const bool recodingFlag)
    : m_codeType (codeType),
      m_field (field),
      m_users (users),
      m_generationSize (generationSize),
      m_packetSize (packetSize),
      m_recodingFlag (recodingFlag),
      m_recodersSockets (recodersSockets)
  {
    srand(static_cast<uint32_t>(time(0)));

    // Call factories from basic parameters

    kodocpp::encoder_factory encoderFactory (m_codeType, m_field,
      m_generationSize, m_packetSize);
    kodocpp::decoder_factory decoderFactory (m_codeType, m_field,
      m_generationSize, m_packetSize);

    // Create encoder and disable systematic mode
    m_encoder = encoderFactory.build ();
    m_encoder.set_systematic_off ();

    // Initialize the encoder data buffer
    m_encoderBuffer.resize (m_encoder.block_size ());
    m_encoder.set_const_symbols (m_encoderBuffer.data (),
        m_encoder.block_size ());
    m_payload.resize (m_encoder.payload_size ());

    // Create recoders and place them in a vector
    m_recoderBuffers.resize (m_users);
    for (uint32_t n = 0; n < m_users; n++)
     {
       kodocpp::decoder recoder = decoderFactory.build ();

       // Create data buffer for the decoder
       m_recoderBuffers[n].resize (recoder.block_size ());
       recoder.set_mutable_symbols(m_recoderBuffers[n].data (),
         recoder.block_size ());
       m_recoders.emplace_back (recoder);
     }

    // Create decoder and its data buffer
    m_decoder = decoderFactory.build ();
    m_decoderBuffers.resize (m_decoder.block_size ());
    m_decoder.set_mutable_symbols (m_decoderBuffers.data (),
        m_decoder.block_size ());

    // Initialize transmission counts
    m_encoderTransmissionCount = 0;
    m_recodersTransmissionCount = 0;
    m_decoderRank = 0;

    m_previousPackets = std::vector<ns3::Ptr<ns3::Packet>> (m_users);
  }

  void SendPacketEncoder (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
  {
    bool allRecodersDecoded = true;

    for (auto recoder : m_recoders)
      {
         allRecodersDecoded &= recoder.is_complete ();
      }

    if (!(m_decoder.is_complete () || (m_recodingFlag && allRecodersDecoded)))
      {
        std::cout << "+-----------------------------------+" << std::endl;
        std::cout << "|Sending a coded packet from ENCODER|" << std::endl;
        std::cout << "+-----------------------------------+" << std::endl;

        uint32_t bytesUsed = m_encoder.write_payload (&m_payload[0]);
        auto packet = ns3::Create<ns3::Packet> (&m_payload[0], bytesUsed);
        socket->Send (packet);
        m_encoderTransmissionCount++;

        ns3::Simulator::Schedule (pktInterval,
          &EncoderRecodersDecoder::SendPacketEncoder, this,
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
        m_previousPackets[id] = packet;
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
            uint32_t bytesUsed = recoder.write_payload (&m_payload[0]);
            auto packet = ns3::Create<ns3::Packet> (&m_payload[0], bytesUsed);
            socket->Send (packet);
            m_recodersTransmissionCount++;
          }
        else
          {
            auto packet = m_previousPackets[id];

            // Remove all packet tags in order to the callback retag
            // them to avoid ~/ns-3-dev/src/common/packet-tag-list.cc,
            // line=139 assert failure. Tag removal is shown in
            // ~/ns-3-dev/src/applications/udp-echo/udp-echo-server.cc
            // for packet forwarding

            packet->RemoveAllPacketTags ();
            socket->Send (packet);
            m_recodersTransmissionCount++;
            std::cout << "Forwarding a previous packet from RECODER...\n"
                      << std::endl;
         }

        ns3::Simulator::Schedule (pktInterval,
          &EncoderRecodersDecoder::SendPacketRecoder, this,
          socket, pktInterval);
      }
  }

  void ReceivePacketDecoder (ns3::Ptr<ns3::Socket> socket)
  {
    auto packet = socket->Recv ();
    packet->CopyData (&m_payload[0], m_decoder.payload_size ());
    m_decoder.read_payload (&m_payload[0]);

    if (m_decoder.rank () > m_decoderRank)
      {
        std::cout << "Received an innovative packet at DECODER!" << std::endl;
        std::cout << "Decoder rank: " << m_decoder.rank () << "\n"
          << std::endl;
        m_decoderRank = m_decoder.rank ();

        if (m_decoder.is_complete ())
          {
            socket->Close ();
            std::cout << "*** Decoding completed! ***" << std::endl;
            std::cout << "Encoder transmissions: "
                      << m_encoderTransmissionCount << std::endl;
            std::cout << "Recoders transmissions: "
                      << m_recodersTransmissionCount << std::endl;
            uint32_t total = m_encoderTransmissionCount +
                             m_recodersTransmissionCount;
            std::cout << "Total transmissions: " << total << std::endl;
          }
      }
  }

private:

  const kodo_code_type m_codeType;
  const kodo_finite_field m_field;
  const uint32_t m_users;
  const uint32_t m_generationSize;
  const uint32_t m_packetSize;
  const bool m_recodingFlag;

  kodocpp::encoder m_encoder;
  std::vector<uint8_t> m_encoderBuffer;
  std::vector<kodocpp::decoder> m_recoders;
  std::vector<std::vector<uint8_t>> m_recoderBuffers;
  kodocpp::decoder m_decoder;
  std::vector<uint8_t> m_decoderBuffers;
  std::vector<ns3::Ptr<ns3::Socket>> m_recodersSockets;

  std::vector<uint8_t> m_payload;
  uint32_t m_encoderTransmissionCount;
  uint32_t m_recodersTransmissionCount;
  uint32_t m_decoderRank;
  std::vector<ns3::Ptr<ns3::Packet>> m_previousPackets;
};
