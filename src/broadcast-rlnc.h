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
 *
 * Author: Néstor J. Hernández M. <nestor@steinwurf.com>
 */

// This object implements RLNC (random linear network coding) in
// the application layer for a broadcsat topology.

#pragma once

#include <kodo/rlnc/full_rlnc_codes.hpp>
#include <kodo/trace.hpp>
#include <kodo/wrap_copy_payload_decoder.hpp>


template<class field, class encoderTrace, class decoderTrace>
class BroadcastRlnc
{
public:

  using rlnc_encoder = typename kodo::full_rlnc_encoder<field, encoderTrace>;
  using non_copy_rlnc_decoder = typename kodo::full_rlnc_decoder<field,
                                                                 decoderTrace>;

  using rlnc_decoder = typename kodo::wrap_copy_payload_decoder<
                                    non_copy_rlnc_decoder>;

  using encoder_pointer = typename rlnc_encoder::factory::pointer;
  using decoder_pointer = typename rlnc_decoder::factory::pointer;

  BroadcastRlnc (const uint32_t users, const uint32_t generationSize,
    const uint32_t packetSize, const std::vector<ns3::Ptr<ns3::Socket>>& sinks)
    : m_users (users),
      m_generationSize (generationSize),
      m_packetSize (packetSize),
      m_sinks (sinks)
  {
    // Call factories from basic parameters
    typename rlnc_encoder::factory encoder_factory (m_generationSize,
      m_packetSize);
    typename rlnc_decoder::factory decoder_factory (m_generationSize,
      m_packetSize);

    // Encoder creation and settings
    m_encoder = encoder_factory.build ();
    m_encoder->set_systematic_off ();
    m_encoder->seed ((uint32_t)time (0));

    // Initialize the input data
    std::vector<uint8_t> data (m_encoder->block_size (), 'x');
    m_encoder->set_symbols (sak::storage (data));
    m_payload_buffer.resize (m_encoder->payload_size ());

    // Decoders creation and settings
    m_decoders = std::vector<decoder_pointer> (m_users);

    for (uint32_t n = 0; n < m_users; n++)
     {
       m_decoders[n] = decoder_factory.build ();
       m_socketMap[m_sinks[n]] = m_decoders[n];
     }

    // Initialize transmission count
    m_transmission_count = 0;
  }

  void SendPacket (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
  {
    bool all_decoded = true;

    for (auto decoder : m_decoders)
      {
        all_decoded = all_decoded && decoder->is_complete ();
      }

    if (!all_decoded)
      {
        std::cout << "+---------------------+" << std::endl;
        std::cout << "|Sending a combination|" << std::endl;
        std::cout << "+---------------------+" << std::endl;
        uint32_t bytes_used = m_encoder->encode (&m_payload_buffer[0]);
        auto packet = ns3::Create<ns3::Packet> (&m_payload_buffer[0],
          bytes_used);
        socket->Send (packet);
        m_transmission_count++;

        if (kodo::has_trace<rlnc_encoder>::value)
          {
            std::cout << "Trace encoder:" << std::endl;
            kodo::trace (m_encoder, std::cout);
          }

        ns3::Simulator::Schedule (pktInterval,
          &BroadcastRlnc <field, encoderTrace, decoderTrace>::SendPacket,
          this, socket, pktInterval);
      }
    else
      {
        std::cout << "Decoding completed! Total transmissions: "
          << m_transmission_count << std::endl;
        socket->Close ();
      }
  }

  void ReceivePacket (ns3::Ptr<ns3::Socket> socket)
  {
    decoder_pointer decoder = m_socketMap[socket];
    auto packet = socket->Recv ();
    packet->CopyData (&m_payload_buffer[0], decoder->payload_size ());
    decoder->decode (&m_payload_buffer[0]);

    if (kodo::has_trace<rlnc_decoder>::value)
      {
        auto filter = [] (const std::string& zone)
        {
          std::set<std::string> filters =
            {"decoder_state","input_symbol_coefficients"};
          return filters.count (zone);
        };

        auto id = std::distance (std::begin (m_socketMap),
          m_socketMap.find (socket)) + 1;

        std::cout << "Received a packet at decoder " << id << std::endl;
        std::cout << "Trace on decoder " << id << " is: " << std::endl;
        kodo::trace (decoder, std::cout, filter);
      }
  }

private:

  const uint32_t m_users;
  const uint32_t m_generationSize;
  const uint32_t m_packetSize;

  encoder_pointer m_encoder;
  std::vector<decoder_pointer> m_decoders;
  std::vector<ns3::Ptr<ns3::Socket>> m_sinks;
  std::map<ns3::Ptr<ns3::Socket>,decoder_pointer> m_socketMap;

  std::vector<uint8_t> m_payload_buffer;
  uint32_t m_transmission_count;
};
