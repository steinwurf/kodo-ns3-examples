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
// the application layer for a broadcast topology.

#pragma once

#include <kodocpp/kodocpp.hpp>

class BroadcastRlnc
{
public:

  BroadcastRlnc (const bool enableTrace, const uint32_t users,
    const uint32_t generationSize, const uint32_t packetSize,
    const ns3::Ptr<ns3::Socket>& source,
    const std::vector<ns3::Ptr<ns3::Socket>>& sinks)
    : m_enableTrace (enableTrace),
      m_users (users),
      m_generationSize (generationSize),
      m_packetSize (packetSize),
      m_source (source),
      m_sinks (sinks)
  {
    srand(static_cast<uint32_t>(time(0)));

    // Call factories from basic parameters
    kodocpp::encoder_factory encoder_factory (kodo_full_vector, kodo_binary8,
      m_generationSize, m_packetSize, m_enableTrace);
    kodocpp::decoder_factory decoder_factory (kodo_full_vector, kodo_binary8,
      m_generationSize, m_packetSize, m_enableTrace);

    // Encoder creation and settings
    kodocpp::encoder encoder = encoder_factory.build ();
    encoder.set_systematic_off ();

    if (encoder.has_set_trace_stdout ())
      {
        encoder.set_trace_stdout ();
      }

    // Data symbols
    std::vector<uint8_t> data_in (encoder.block_size (), 'x');
    encoder.set_symbols (data_in.data (), encoder.payload_size ());
    m_payload.resize (encoder.payload_size ());

    m_encoder.emplace_back (encoder);

    // Decoders creation and settings
    for (uint32_t n = 0; n < m_users; n++)
      {
        m_decoders.emplace_back (decoder_factory.build ());
      }

    // Initialize transmission count
    m_transmissionCount = 0;
  }

  void SendPacket (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
  {
    bool all_decoded = true;

    for (uint32_t n = 0; n < m_users; n++)
      {
        all_decoded = all_decoded && m_decoders.at (n).is_complete ();
      }

    if (!all_decoded)
      {
        std::cout << "+---------------------+" << std::endl;
        std::cout << "|Sending a combination|" << std::endl;
        std::cout << "+---------------------+" << std::endl;
        uint32_t bytes_used = m_encoder.at (0).write_payload (&m_payload[0]);
        auto packet = ns3::Create<ns3::Packet> (&m_payload[0], bytes_used);
        socket->Send (packet);
        m_transmissionCount++;

        ns3::Simulator::Schedule (pktInterval, &BroadcastRlnc::SendPacket,
          this, socket, pktInterval);
      }
    else
      {
        std::cout << "Decoding completed! Total transmissions: "
          << m_transmissionCount << std::endl;
        socket->Close ();
      }
  }

  void ReceivePacket (ns3::Ptr<ns3::Socket> socket)
  {
    // Find the socket index
    auto it = std::find(m_sinks.begin (), m_sinks.end (), socket);
    auto n = std::distance (m_sinks.begin (), it);
    std::vector<uint8_t> data;
    data.resize (m_decoders.at (n).payload_size ());

    // Use the index to generate the packet
    auto packet = socket->Recv ();
    packet->CopyData (&data[0], m_decoders.at (n).payload_size ());
    m_decoders.at (n).read_payload (&data[0]);

    std::cout << "Received a packet at decoder " << n + 1 << std::endl;

    if (m_decoders.at (n).has_set_trace_callback ())
      {
        auto callback = [](const std::string& zone, const std::string& data)
        {
          std::set<std::string> filters =
            {"decoder_state","input_symbol_coefficients"};
          if (filters.count (zone))
            {
              std::cout << zone << ":" << std::endl;
              std::cout << data << std::endl;
            }
        };

        m_decoders.at (n).set_trace_callback (callback);
      }
  }

private:

  const bool m_enableTrace;
  const uint32_t m_users;
  const uint32_t m_generationSize;
  const uint32_t m_packetSize;

  ns3::Ptr<ns3::Socket> m_source;
  std::vector<ns3::Ptr<ns3::Socket>> m_sinks;
  std::vector<kodocpp::encoder> m_encoder;
  std::vector<kodocpp::decoder> m_decoders;

  std::vector<uint8_t> m_payload;
  uint32_t m_transmissionCount;
};