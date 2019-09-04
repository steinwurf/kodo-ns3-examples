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

// This class is a container for kodo encoders and decoders that operate
// over multiple blocks of data.

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <endian/big_endian.hpp>
#include <kodo_core/object/storage_decoder.hpp>
#include <kodo_core/object/storage_encoder.hpp>
#include <kodo_rlnc/coders.hpp>

class BroadcastObject
{
private:

  using encoder = kodo_rlnc::encoder;
  using decoder = kodo_rlnc::decoder;

  using storage_encoder = kodo_core::object::storage_encoder<encoder>;
  using storage_decoder = kodo_core::object::storage_decoder<decoder>;

  using encoder_stack_ptr = storage_encoder::stack_pointer;
  using decoder_stack_ptr = storage_decoder::stack_pointer;

public:

  BroadcastObject (const fifi::finite_field field, const uint32_t users,
    const uint32_t objectSize, const uint32_t generationSize,
    const uint32_t packetSize, const uint32_t extraPackets,
    const ns3::Ptr<ns3::Socket>& source,
    const std::vector<ns3::Ptr<ns3::Socket>>& sinks)
    : m_field (field),
      m_users (users),
      m_objectSize (objectSize),
      m_generationSize (generationSize),
      m_packetSize (packetSize),
      m_extraPackets (extraPackets),
      m_source (source),
      m_sinks (sinks)
  {
    m_currentBlock = 0;
    m_payloadsForCurrentBlock = 0;

    // Create the storage encoder
    m_encoder = storage_encoder (m_field, m_generationSize, m_packetSize);

    // Initialize the data buffer that might be larger than a single generation
    // In a realistic application, this input buffer can be read from a file
    m_encoderBuffer.resize (m_objectSize);
    m_encoder.set_const_storage (storage::storage (m_encoderBuffer));
    m_encoderStacks.resize (m_encoder.blocks ());

    // Create storage decoders for each sink node
    m_decoderBuffers.resize (m_users);
    m_decoderStacks.resize (m_users);
    for (uint32_t n = 0; n < m_users; n++)
      {
        storage_decoder decoder (m_field, m_generationSize, m_packetSize);

        // Create data buffer for the decoder
        m_decoderBuffers[n].resize (m_objectSize);
        decoder.set_mutable_storage (storage::storage (m_decoderBuffers[n]));
        m_decoderStacks[n].resize (decoder->blocks ());

        m_decoders.emplace_back (decoder);
      }

    // Initialize transmission count
    m_transmissionCount = 0;
  }

  void SendPacket (ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
  {
    bool allDecoded = true;

    for (uint32_t n = 0; n < m_users; n++)
      {
        allDecoded = allDecoded && m_decoders[n].is_complete ();
      }

    if (!allDecoded)
      {
        // Move to the next block if the encoder already sent sufficient
        // packets for the current block (in a realistic application, we could
        // use feedback information from the receiver about the incomplete
        // blocks)
        if (m_payloadsForCurrentBlock >= m_generationSize + m_extraPackets)
          {
            m_currentBlock = (m_currentBlock + 1) % m_encoder.blocks ();
            m_payloadsForCurrentBlock = 0;
          }

        uint32_t i = m_currentBlock;

        // Create the encoder for this block if necessary
        if (!m_encoderStacks[i])
          {
            m_encoderStacks[i] = m_encoder.build (i);
            std::cout << "Encoder created for block: " << i << std::endl;
          }

        // Create a payload that will also contain the block ID
        std::vector<uint8_t> payload (4 + m_encoderStacks[i]->payload_size ());
        // First, the current block ID to the payload
        endian::big_endian::put<uint32_t>(i, payload.data());
        // Write a symbol to the payload buffer after the block ID
        uint32_t bytesUsed = m_encoderStacks[i]->write_payload (&payload[4]);
        auto packet = ns3::Create<ns3::Packet> (&payload[0], 4 + bytesUsed);
        socket->Send (packet);
        m_payloadsForCurrentBlock++;
        m_transmissionCount++;

        ns3::Simulator::Schedule (pktInterval, &BroadcastObject::SendPacket,
          this, socket, pktInterval);
      }
    else
      {
        std::cout << "Simulation completed! Total transmissions: "
          << m_transmissionCount << std::endl;
        socket->Close ();
      }
  }

  void ReceivePacket (ns3::Ptr<ns3::Socket> socket)
  {
    // Find the decoder index based on the socket
    auto it = std::find(m_sinks.begin (), m_sinks.end (), socket);
    auto n = std::distance (m_sinks.begin (), it);

    // Pass the packet payload to the appropriate decoder
    auto packet = socket->Recv ();
    uint32_t size = packet->GetSize ();
    std::vector<uint8_t> payload (size);
    packet->CopyData (&payload[0], size);

    // Read block ID from the payload
    uint32_t i = 0;
    endian::big_endian::get<uint32_t>(i, payload.data());

    // Create the decoder for this block if it does not exist
    if (!m_decoderStacks[n][i])
      {
        m_decoderStacks[n][i] = m_decoders[n].build (i);
      }

    // Nothing to do if this block is already completed
    if (m_decoderStacks[n][i]->is_complete ())
        return;

    // Pass the symbol to the appropriate decoder
    m_decoderStacks[n][i]->read_payload (&payload[4]);

    if (m_decoderStacks[n][i]->is_complete ())
      {
        std::cout << "Block " << i << " completed at Decoder " << n + 1
                  << std::endl;
      }
    if (m_decoders[n].is_complete ())
      {
        std::cout << "All blocks completed at Decoder " << n + 1 << std::endl;
      }
  }

private:

  uint32_t m_currentBlock;
  uint32_t m_payloadsForCurrentBlock;
  const fifi::finite_field m_field;
  const uint32_t m_users;
  const uint32_t m_objectSize;
  const uint32_t m_generationSize;
  const uint32_t m_packetSize;
  const uint32_t m_extraPackets;

  ns3::Ptr<ns3::Socket> m_source;
  std::vector<ns3::Ptr<ns3::Socket>> m_sinks;
  storage_encoder m_encoder;
  std::vector<encoder_stack_ptr> m_encoderStacks;
  std::vector<uint8_t> m_encoderBuffer;
  std::vector<storage_decoder> m_decoders;
  std::vector<std::vector<decoder_stack_ptr>> m_decoderStacks;
  std::vector<std::vector<uint8_t>> m_decoderBuffers;

  uint32_t m_transmissionCount;
};
