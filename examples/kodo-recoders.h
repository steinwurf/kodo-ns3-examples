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

#include <cstdint>
#include <memory>
#include <vector>

#include <endian/big_endian.hpp>
#include <kodo/block/decoder.hpp>
#include <kodo/block/encoder.hpp>
#include <kodo/block/generator/random_uniform.hpp>
#include <kodo/finite_field.hpp>

class Recoders
{
public:
    Recoders(const kodo::finite_field field, const uint32_t users,
             const uint32_t generationSize, const uint32_t packetSize,
             const std::vector<ns3::Ptr<ns3::Socket>>& recodersSockets,
             const bool recodingFlag, const double transmitProbability) :
        m_field(field),
        m_users(users), m_generationSize(generationSize),
        m_packetSize(packetSize), m_recodingFlag(recodingFlag),
        m_transmitProbability(transmitProbability),
        m_recodersSockets(recodersSockets), m_encoder(field), m_decoder(field),
        m_generator(field)
    {
        m_payload.resize(packetSize);
        m_generator.configure(m_generationSize);
        m_generator.set_seed(rand());
        m_coefficients.resize(m_generator.max_coefficients_bytes());
        auto symbol_bytes = packetSize - m_generator.max_coefficients_bytes();

        // Create encoder and disable systematic mode
        m_encoder.configure(m_generationSize, symbol_bytes);

        // Initialize the encoder data buffer
        m_encoderBuffer.resize(m_encoder.block_bytes());
        m_encoder.set_symbols_storage(m_encoderBuffer.data());

        // Create recoders and place them in a vector
        m_recoderBuffers.resize(m_users);
        for (uint32_t n = 0; n < m_users; n++)
        {
            m_recoders.emplace_back(m_field);
            auto& recoder = m_recoders.back();
            recoder.configure(m_generationSize, symbol_bytes);

            // Create data buffer for the decoder
            m_recoderBuffers[n].resize(recoder.block_bytes());
            recoder.set_symbols_storage(m_recoderBuffers[n].data());
        }

        // Create decoder and its data buffer
        m_decoder.configure(m_generationSize, symbol_bytes);
        m_decoderBuffer.resize(m_decoder.block_bytes());
        m_decoder.set_symbols_storage(m_decoderBuffer.data());

        // Initialize transmission counts
        m_encoderTransmissionCount = 0;
        m_recodersTransmissionCount = 0;
        m_decoderRank = 0;

        // Initialize previous packets buffer
        // m_previousPackets = std::vector<ns3::Ptr<ns3::Packet>> (m_users);

        m_uniformRandomVariable =
            ns3::CreateObject<ns3::UniformRandomVariable>();
        m_uniformRandomVariable->SetAttribute("Min", ns3::DoubleValue(0.0));
        m_uniformRandomVariable->SetAttribute("Max", ns3::DoubleValue(1.0));
    }

    void SendPacketEncoder(ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
    {
        bool allRecodersDecoded = true;

        for (const auto& recoder : m_recoders)
        {
            allRecodersDecoded &= recoder.is_complete();
        }

        if (!allRecodersDecoded)
        {
            std::cout << "+-----------------------------------+" << std::endl;
            std::cout << "|Sending a coded packet from ENCODER|" << std::endl;
            std::cout << "+-----------------------------------+" << std::endl;

            m_generator.generate(m_payload.data());

            m_encoder.encode_symbol(m_payload.data() +
                                        m_generator.max_coefficients_bytes(),
                                    m_coefficients.data());
            auto packet =
                ns3::Create<ns3::Packet>(m_payload.data(), m_payload.size());
            socket->Send(packet);
            m_encoderTransmissionCount++;

            ns3::Simulator::Schedule(pktInterval, &Recoders::SendPacketEncoder,
                                     this, socket, pktInterval);
        }
        else
        {
            socket->Close();
        }
    }

    void ReceivePacketRecoder(ns3::Ptr<ns3::Socket> socket)
    {
        // Find the recoder index based on the socket
        auto it = std::find(m_recodersSockets.begin(), m_recodersSockets.end(),
                            socket);
        auto id = std::distance(m_recodersSockets.begin(), it);

        std::cout << "Received a packet at RECODER " << id + 1 << std::endl;

        auto& recoder = m_recoders[id];

        auto packet = socket->Recv();
        packet->CopyData(m_payload.data(), packet->GetSize());

        recoder.decode_symbol(m_payload.data() +
                                  m_generator.max_coefficients_bytes(),
                              m_payload.data());

        // Keep track of the received packets for each recoder
        // when no recoding is employed to forward one of them
        // uniformly at random
        if (!m_recodingFlag)
        {
            m_previousPackets[id].push_back(packet);
        }

        if (recoder.is_complete())
        {
            std::cout << "RECODER " << id + 1 << " is complete!\n" << std::endl;
        }
    }

    void SendPacketRecoder(ns3::Ptr<ns3::Socket> socket, ns3::Time pktInterval)
    {
        // Find the recoder index based on the socket
        auto it = std::find(m_recodersSockets.begin(), m_recodersSockets.end(),
                            socket);
        auto id = std::distance(m_recodersSockets.begin(), it);

        kodo::block::decoder& recoder = m_recoders[id];

        // A node wil transmit at random with probability
        // m_transmitProbability. Thus, we throw a coin
        // (Bernoulli Random Variable) with this probability
        // and check it. ns-3 does not have Bernoulli but has
        // Uniform. So, we convert this random variable through
        // the inverse transform method.

        bool transmit = false;

        if (m_uniformRandomVariable->GetValue() <= m_transmitProbability)
        {
            transmit = true;
        }

        if (!m_decoder.is_complete() && recoder.rank() > 0 && transmit)
        {
            if (m_recodingFlag)
            {
                std::cout << "+-------------------------------------+"
                          << std::endl;
                std::cout << "|Sending a coded packet from RECODER " << id + 1
                          << "|" << std::endl;
                std::cout << "+-------------------------------------+"
                          << std::endl;

                // Recode a new packet and send it

                m_generator.generate_recode(m_coefficients.data(), recoder);

                recoder.recode_symbol(m_payload.data() +
                                          m_generator.max_coefficients_bytes(),
                                      m_payload.data(), m_coefficients.data());

                auto packet = ns3::Create<ns3::Packet>(m_payload.data(),
                                                       m_payload.size());
                socket->Send(packet);
                m_recodersTransmissionCount++;
            }
            else
            {
                std::cout << "+-------------------------------------------+"
                          << std::endl;
                std::cout << "|Forwarding a previous packet from RECODER "
                          << id + 1 << "|" << std::endl;
                std::cout << "+-------------------------------------------+"
                          << std::endl;

                // Get a previously received packet uniformly at random and
                // forward it
                uint32_t max = m_previousPackets[id].size();
                uint32_t randomIndex =
                    m_uniformRandomVariable->GetInteger(0, max - 1);
                auto packet = m_previousPackets[id][randomIndex];

                // Remove all packet tags in order to the callback retag
                // them to avoid ~/ns-3-dev/src/common/packet-tag-list.cc,
                // line=139 assert failure. Tag removal is shown in
                // ~/ns-3-dev/src/applications/udp-echo/udp-echo-server.cc
                // for packet forwarding

                packet->RemoveAllPacketTags();
                socket->Send(packet);
                m_recodersTransmissionCount++;
            }
        }

        // Schedule the next packet
        if (!m_decoder.is_complete())
        {
            ns3::Simulator::Schedule(pktInterval, &Recoders::SendPacketRecoder,
                                     this, socket, pktInterval);
        }
    }

    void ReceivePacketDecoder(ns3::Ptr<ns3::Socket> socket)
    {
        auto packet = socket->Recv();
        packet->CopyData(m_payload.data(), packet->GetSize());

        m_decoder.decode_symbol(m_payload.data() +
                                    m_generator.max_coefficients_bytes(),
                                m_payload.data());

        if (m_decoder.rank() > m_decoderRank)
        {
            std::cout << "Received an innovative packet at DECODER!"
                      << std::endl;
            std::cout << "Decoder rank: " << m_decoder.rank() << "\n"
                      << std::endl;
            m_decoderRank = m_decoder.rank();

            if (m_decoder.is_complete())
            {
                socket->Close();
                std::cout << "*** Decoding completed! ***" << std::endl;
                std::cout << "Encoder transmissions: "
                          << m_encoderTransmissionCount << std::endl;
                std::cout << "Recoders transmissions: "
                          << m_recodersTransmissionCount << std::endl;
                uint32_t total =
                    m_encoderTransmissionCount + m_recodersTransmissionCount;
                std::cout << "Total transmissions: " << total << std::endl;
            }
        }
    }

private:
    const kodo::finite_field m_field;
    const uint32_t m_users;
    const uint32_t m_generationSize;
    const uint32_t m_packetSize;
    const bool m_recodingFlag;
    const double m_transmitProbability;

    std::vector<ns3::Ptr<ns3::Socket>> m_recodersSockets;
    kodo::block::encoder m_encoder;
    std::vector<uint8_t> m_encoderBuffer;
    std::vector<kodo::block::decoder> m_recoders;
    std::vector<std::vector<uint8_t>> m_recoderBuffers;
    kodo::block::decoder m_decoder;
    std::vector<uint8_t> m_decoderBuffer;

    std::vector<uint8_t> m_payload;
    uint32_t m_encoderTransmissionCount;
    uint32_t m_recodersTransmissionCount;
    uint32_t m_decoderRank;
    std::map<uint32_t, std::vector<ns3::Ptr<ns3::Packet>>> m_previousPackets;

    ns3::Ptr<ns3::UniformRandomVariable> m_uniformRandomVariable;

    kodo::block::generator::random_uniform m_generator;
    std::vector<uint8_t> m_coefficients;
};
