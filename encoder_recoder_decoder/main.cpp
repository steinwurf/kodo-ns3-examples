/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
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
 */

// This example shows how to use the Kodo library for recoding at an
// intermediate node in the network within a ns-3 simulation. Recoding is
// one of the characteristic features of network coding because it
// differentiates from end-to-end codes by allowing any intermediate node
// to recode a coded packet, even if its data set has not been completely
// decoded yet.

// In the script below an encoder transmits coded packets from a block of
// data to a node through an erasure channel with a given rate, which may or
// may not recode the data. This node transmits its packets to a decoder
// (thorugh another erasure channel) until it has received a complete
// generation. Here the packets are sent using the binary field, GF(2) with a
// generation of 5 packets and 1000 (application) bytes to the other node.
// Topology with IP addresses per net device is as follows:

//         +-----------+  e1  +-----------+  e2  +------------+
//         |  encoder  |+---->|  recoder  |+---->|  decoder_2 |
//         +-----------+      +-----------+      +------------+
//  IP:       10.1.1.1           10.1.1.2           10.1.1.4
//                               10.1.1.3

// In the previous figure: e1 is the packet error rate between encoder and
// recoder, namely errorRateEncoderRecoder. e2 is the packet error rate
// between recoder and decoder, namely errorRateRecoderDecoder. Each IP address
// represents a net device in the node.

// You can change any parameter, by running (for example with a different
// generation size):
// ./build/linux/encoder_recoder_decoder/encoder_recoder_decoder --generationSize=GENERATION_SIZE

// When you are done, you will notice four pcap trace files in your directory.
// You can review the files with Wireshark or tcpdump. If you have tcpdump
// installed, you can try (for example) this:
//
// tcpdump -r multihop-0-0.pcap -nn -tt


#include <ns3/core-module.h>
#include <ns3/point-to-point-star.h>
#include <ns3/network-module.h>
#include <ns3/config-store-module.h>
#include <ns3/internet-module.h>

#include <kodo/rlnc/full_rlnc_codes.hpp>
#include <kodo/trace.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>

using namespace ns3;

// The encoder / decoder type we will use. Here we consider GF(2). For GF(2^8)
// just change "binary" for "binary8"

// Also we implement the Kodo traces (available since V.17.0.0). Here, we have
// enabled the decoder trace and disabled the encoder trace.
typedef kodo::full_rlnc_encoder<fifi::binary8,kodo::disable_trace> rlnc_encoder;
typedef kodo::full_rlnc_decoder<fifi::binary8,kodo::enable_trace> rlnc_decoder;

// Just for illustration purposes, this object implements both
// the encoder, recoder and decoder.
class KodoSimulation
{
public:

  KodoSimulation(const rlnc_encoder::pointer& encoder,
                 const rlnc_decoder::pointer& recoder,
                 const rlnc_decoder::pointer& decoder,
                 const bool recoding_flag)
    : m_encoder(encoder),
      m_recoder(recoder),
      m_decoder(decoder),
      m_recoding_flag(recoding_flag)
  {
    m_encoder->set_systematic_off();
    m_encoder->seed(time(0));

    // Initialize the input data
    std::vector<uint8_t> data(encoder->block_size(), 'x');
    m_encoder->set_symbols(sak::storage(data));

    m_payload_buffer.resize(m_encoder->payload_size());
    m_encoder_transmission_count = 0;
    m_recoder_transmission_count = 0;
  }

  void ReceivePacketRecoder (Ptr<Socket> socket)
  {
    auto packet = socket->Recv();
    packet->CopyData(&m_payload_buffer[0], m_recoder->payload_size());
    m_recoder->decode(&m_payload_buffer[0]);
    std::cout << "Received one packet at recoder" << std::endl;

    if (kodo::has_trace<rlnc_decoder>::value)
      {
        auto filter = [](const std::string& zone)
        {
          std::set<std::string> filters =
            {"decoder_state","input_symbol_coefficients"};
          return filters.count(zone);
        };

        std::cout << "Trace recoder:" << std::endl;
        kodo::trace(m_recoder, std::cout, filter);
      }

    if (m_recoding_flag)
      {
        uint32_t bytes_used = m_recoder->recode(&m_payload_buffer[0]);
        auto recodedPacket = Create<Packet> (&m_payload_buffer[0], bytes_used);
        std::cout << "Recoded one packet at recoder" << std::endl;
        m_recoder_transmission_count++;
        socket->Send (recodedPacket);
      }
    else
      {
        // Remove all packet tags in order to the callback retag them to avoid
        // ~/ns-3-dev/src/common/packet-tag-list.cc, line=139 assert failure.
        // Tag removal is shown in ~/ns-3-dev/src/applications/udp-echo/
        // udp-echo-server.cci for packet forwarding

        packet->RemoveAllPacketTags ();
        socket->Send (packet);
        m_recoder_transmission_count++;
        std::cout << "Forwarded one packet at recoder" << std::endl;
      }
  }

  void ReceivePacketDecoder (Ptr<Socket> socket)
  {
    auto packet = socket->Recv();
    packet->CopyData(&m_payload_buffer[0], m_decoder->payload_size());
    m_decoder->decode(&m_payload_buffer[0]);
    std::cout << "Received one packet at decoder" << std::endl;

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

  void GenerateTraffic (Ptr<Socket> socket, Time pktInterval)
  {
    if (!m_decoder->is_complete())
      {
        std::cout << "Sending a combination" << std::endl;
        uint32_t bytes_used = m_encoder->encode(&m_payload_buffer[0]);
        auto packet = Create<Packet> (&m_payload_buffer[0], bytes_used);
        socket->Send (packet);
        m_encoder_transmission_count++;

        if (kodo::has_trace<rlnc_encoder>::value)
          {
            std::cout << "Trace encoder:" << std::endl;
            kodo::trace(m_encoder, std::cout);
          }

        Simulator::Schedule (pktInterval, &KodoSimulation::GenerateTraffic,
                             this, socket, pktInterval);
      }
    else
      {
        std::cout << "Decoding completed! " << std::endl;
        std::cout << "Encoder transmissions: " << m_encoder_transmission_count
                  << std::endl;
        std::cout << "Recoder transmissions: " << m_recoder_transmission_count
                  << std::endl;
        std::cout << "Total transmissions: "
                  << m_encoder_transmission_count + m_recoder_transmission_count
                  << std::endl;
        socket->Close ();
      }
  }

private:

  rlnc_encoder::pointer m_encoder;
  rlnc_decoder::pointer m_recoder;
  rlnc_decoder::pointer m_decoder;

  std::vector<uint8_t> m_payload_buffer;
  uint32_t m_encoder_transmission_count;
  uint32_t m_recoder_transmission_count;
  bool m_recoding_flag;
};

int main (int argc, char *argv[])
{
  uint32_t packetSize = 1000; // Application bytes per packet
  double interval = 1.0; // Time between events
  uint32_t generationSize = 5; // RLNC generation size
  double errorRateEncoderRecoder = 0.4; // Error rate for encoder-recoder link
  double errorRateRecoderDecoder = 0.2; // Error rate for recoder-decoder link
  bool recodingFlag = true; // Flag to control recoding

  Time interPacketInterval = Seconds (interval);

  CommandLine cmd;

  cmd.AddValue ("packetSize", "Size of application packet sent", packetSize);
  cmd.AddValue ("interval", "Interval (seconds) between packets", interval);
  cmd.AddValue ("generationSize", "Set the generation size to use",
                generationSize);
  cmd.AddValue ("errorRateEncoderRecoder",
                "Packet erasure rate for the encoder-recoder link",
                errorRateEncoderRecoder);
  cmd.AddValue ("errorRateRecoderDecoder",
                "Packet erasure rate for the recoder-decoder link",
                errorRateRecoderDecoder);
  cmd.AddValue ("recodingFlag", "Enable packet recoding", recodingFlag);
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);

  // Set the basic helper for a single link
  PointToPointHelper pointToPoint;

  // Create node containers
  NodeContainer nodes;
  nodes.Create (3);
  NodeContainer encoderRecoder = NodeContainer (nodes.Get (0), nodes.Get (1));
  NodeContainer recoderDecoder = NodeContainer (nodes.Get (1), nodes.Get (2));

  // Internet stack for the nodes
  InternetStackHelper internet;
  internet.Install (nodes);

  // Create net device containers
  NetDeviceContainer encoderRecoderDevs = pointToPoint.Install (encoderRecoder);
  NetDeviceContainer recoderDecoderDevs = pointToPoint.Install (recoderDecoder);

  NetDeviceContainer devices = NetDeviceContainer (encoderRecoderDevs,
                                                   recoderDecoderDevs);
  // Set IP addresses
  Ipv4AddressHelper ipv4("10.1.1.0", "255.255.255.0");
  ipv4.Assign (devices);

  // Turn on global static routing so we can actually be routed across the hops
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Do pcap tracing on all point-to-point devices on all nodes. File naming
  // convention is: multihop-[NODE_NUMBER]-[DEVICE_NUMBER].pcap
  pointToPoint.EnablePcapAll ("multihop");

  // Set error model for the net devices
  Config::SetDefault ("ns3::RateErrorModel::ErrorUnit",
                      StringValue ("ERROR_UNIT_PACKET"));

  Ptr<RateErrorModel> errorEncoderRecoder = CreateObject<RateErrorModel> ();
  errorEncoderRecoder->SetAttribute ("ErrorRate",
                                     DoubleValue (errorRateEncoderRecoder));
  devices.Get (1)->SetAttribute ("ReceiveErrorModel",
                                 PointerValue (errorEncoderRecoder));

  Ptr<RateErrorModel> errorRecoderDecoder = CreateObject<RateErrorModel> ();
  errorRecoderDecoder->SetAttribute ("ErrorRate",
                                     DoubleValue (errorRateRecoderDecoder));
  devices.Get (3)->SetAttribute ("ReceiveErrorModel",
                                 PointerValue (errorRecoderDecoder));
  errorEncoderRecoder->Enable ();
  errorRecoderDecoder->Enable ();

  // Creation of RLNC encoder and decoder objects
  rlnc_encoder::factory encoder_factory(generationSize, packetSize);
  rlnc_decoder::factory decoder_factory(generationSize, packetSize);

  // The member build function creates differents instances of each object
  KodoSimulation kodoSimulator(encoder_factory.build(),
                               decoder_factory.build(),
                               decoder_factory.build(),
                               recodingFlag);

  // Setting up application sockets for recoder and decoder
  uint16_t port = 80;
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  // Get node Ipv4 addresses
  Ipv4Address recoderAddress = nodes.Get (1)->GetObject<Ipv4>()->
                                 GetAddress(1,0).GetLocal();
  Ipv4Address decoderAddress = nodes.Get (2)->GetObject<Ipv4>()->
                                 GetAddress(1,0).GetLocal();
  // Socket connection addresses
  InetSocketAddress recoderSocketAddress = InetSocketAddress (recoderAddress,
                                                              port);
  InetSocketAddress decoderSocketAddress = InetSocketAddress (decoderAddress,
                                                              port);
  // Socket bind address
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny(), port);

  // Encoder
  Ptr<Socket> encoderSocket = Socket::CreateSocket (nodes.Get (0), tid);
  encoderSocket->Connect (recoderSocketAddress);

  // Recoder
  Ptr<Socket> recoderSocket = Socket::CreateSocket (nodes.Get (1), tid);
  recoderSocket->Bind(local);
  recoderSocket->Connect (decoderSocketAddress);

  recoderSocket->
    SetRecvCallback (MakeCallback (&KodoSimulation::ReceivePacketRecoder,
                                   &kodoSimulator));
  // Decoder
  Ptr<Socket> decoderSocket = Socket::CreateSocket (nodes.Get (2), tid);
  decoderSocket->Bind(local);
  decoderSocket->
    SetRecvCallback (MakeCallback (&KodoSimulation::ReceivePacketDecoder,
                                   &kodoSimulator));
  // Simulation setup
  Simulator::ScheduleWithContext (encoderSocket->GetNode ()->GetId (),
                                  Seconds (1.0),
                                  &KodoSimulation::GenerateTraffic,
                                  &kodoSimulator, encoderSocket,
                                  interPacketInterval);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
