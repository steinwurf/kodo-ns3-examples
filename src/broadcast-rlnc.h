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

// #include <kodo/wrap_copy_payload_decoder.hpp>
// #include <cstdlib>

#include <kodocpp/kodocpp.hpp>

class BroadcastRlnc
{
public:

  // using rlnc_encoder = typename kodo::full_rlnc_encoder<field, encoderTrace>;
  // using non_copy_rlnc_decoder = typename kodo::full_rlnc_decoder<field,
  //   decoderTrace>;

  // using rlnc_decoder = typename kodo::wrap_copy_payload_decoder<
  //   non_copy_rlnc_decoder>;

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
    kodocpp::encoder_factory encoder_factory (kodo_full_rlnc, kodo_binary,
      m_generationSize, m_packetSize, m_enableTrace);
    kodocpp::decoder_factory decoder_factory (kodo_full_rlnc, kodo_binary,
      m_generationSize, m_packetSize, m_enableTrace);

    // Encoder creation and settings
    m_encoderMap.emplace (m_source, encoder_factory.build ());
    m_encoderMap.at (m_source).set_systematic_off ();

    // Initialize the input with any data and save it in the map
    std::vector<uint8_t> data_in (m_encoderMap.at (m_source).block_size (),
      'x');
    m_encoderMap.at (m_source).set_symbols (data_in.data (),
      m_encoderMap.at (m_source).payload_size ());

    // Decoders creation and settings
    for (uint32_t n = 0; n < m_users; n++)
     {
       m_decodersMap.emplace (m_sinks[n], decoder_factory.build ());
     }

    // Initialize transmission count
    m_transmissionCount = 0;
    std::cout << "Class constructed" << std::endl;
  }

  void SendPacket (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
  {
    bool all_decoded = true;

    for (uint32_t n = 0; n < m_users; n++)
      {
        kodocpp::decoder decoder = m_decodersMap.at (m_sinks[n]);
        all_decoded = all_decoded && decoder.is_complete ();
      }

    if (!all_decoded)
      {
        std::cout << "+---------------------+" << std::endl;
        std::cout << "|Sending a combination|" << std::endl;
        std::cout << "+---------------------+" << std::endl;
        uint32_t bytes_used = m_encoderMap.at (socket).write_payload (
          &m_payload[0]);
        auto packet = ns3::Create<ns3::Packet> (&m_payload[0],
          bytes_used);
        socket->Send (packet);
        m_transmissionCount++;

        // if (kodo::has_trace<rlnc_encoder>::value)
        //   {
        //     std::cout << "Trace encoder:" << std::endl;
        //     kodo::trace (encoder, std::cout);
        //   }

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
    kodocpp::decoder decoder = m_decodersMap.at (socket);
    auto packet = socket->Recv ();
    packet->CopyData (&m_payload[0], decoder.payload_size ());
    decoder.read_payload (&m_payload[0]);

    auto id = std::distance (std::begin (m_decodersMap),
      m_decodersMap.find (socket)) + 1;

    std::cout << "Received a packet at decoder " << id << std::endl;

    // if (kodo::has_trace<rlnc_decoder>::value)
    //   {
    //     auto filter = [] (const std::string& zone)
    //     {
    //       std::set<std::string> filters =
    //         {"decoder_state","input_symbol_coefficients"};
    //       return filters.count (zone);
    //     };

    //     std::cout << "Trace on decoder " << id << " is: " << std::endl;
    //     kodo::trace (decoder, std::cout, filter);
    //   }
  }

private:

  const bool m_enableTrace;
  const uint32_t m_users;
  const uint32_t m_generationSize;
  const uint32_t m_packetSize;

  ns3::Ptr<ns3::Socket> m_source;
  std::vector<ns3::Ptr<ns3::Socket>> m_sinks;
  std::map<ns3::Ptr<ns3::Socket>,kodocpp::encoder> m_encoderMap;
  std::map<ns3::Ptr<ns3::Socket>,kodocpp::decoder> m_decodersMap;

  std::vector<uint8_t> m_payload;
  uint32_t m_transmissionCount;
};