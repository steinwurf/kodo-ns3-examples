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

// This example shows how to use the Kodo library in a broadcast scenario
// within a ns-3 simulation. The code below is based on the wifi_broadcast
// example, which can be found in this repository.

// In the script below the sender transmits encoded packets from a block of
// data to two receivers with the same packet erasure rate. The sender
// continues until all receivers have received all packets. Here the packets
// are sent using the binary field, GF(2) with a generation of 5 packets and
// 1000 (application) bytes to the other node.

// You can change any parameter, by running (for example with a different
// generation size):
// ./build/linux/wired_broadcast/wired_broadcast --generationSize=GENERATION_SIZE

// When you are done, you will notice four pcap trace files in your directory.
// You can review the files with Wireshark or tcpdump. If you have tcpdump
// installed, you can try (for example) this:
//
// tcpdump -r star-0-0.pcap -nn -tt

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
typedef kodo::full_rlnc_encoder<fifi::binary,kodo::disable_trace> rlnc_encoder;
typedef kodo::full_rlnc_decoder<fifi::binary,kodo::enable_trace> rlnc_decoder;

// Just for illustration purposes, this simple objects implements both
// the sender (encoder) and receiver (decoder).
class KodoSimulation
{
public:

  KodoSimulation(const rlnc_encoder::pointer& encoder,
                 const rlnc_decoder::pointer& recoder,
                 const rlnc_decoder::pointer& decoder)
    : m_encoder(encoder),
      m_recoder(recoder),
      m_decoder(decoder)
  {
    m_encoder->set_systematic_off();
    m_encoder->seed(time(0));

    // Initialize the input data
    std::vector<uint8_t> data(encoder->block_size(), 'x');
    m_encoder->set_symbols(sak::storage(data));

    m_payload_buffer.resize(m_encoder->payload_size());
    m_transmission_count = 0;
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
  }

  void ReceivePacketDecoder (Ptr<Socket> socket)
  {
    auto packet = socket->Recv();
    packet->CopyData(&m_payload_buffer[0], m_decoder_2->payload_size());
    m_decoder_2->decode(&m_payload_buffer[0]);
    std::cout << "Received one packet at decoder" << std::endl;

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

  void GenerateTraffic (Ptr<Socket> socket, Time pktInterval )
  {
    if (!m_decoder->is_complete())
      {
        std::cout << "Sending a combination" << std::endl;
        uint32_t bytes_used = m_encoder->encode(&m_payload_buffer[0]);
        auto packet = Create<Packet> (&m_payload_buffer[0], bytes_used);
        socket->Send (packet);
        m_transmission_count++;

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
        std::cout << "Decoding completed! Total transmissions: "
                  << m_transmission_count << std::endl;
        socket->Close ();
      }
  }

private:

  rlnc_encoder::pointer m_encoder;
  rlnc_decoder::pointer m_recoder;
  rlnc_decoder::pointer m_decoder;

  std::vector<uint8_t> m_payload_buffer;
  uint32_t m_transmission_count;
  bool m_recoding_flag;
};

int main (int argc, char *argv[])
{
  uint32_t packetSize = 1000; // Application bytes per packet
  double interval = 1.0; // Time between events
  uint32_t generationSize = 5; // RLNC generation size
  //double errorRate = 0.3; // Error rate for all the links

  Time interPacketInterval = Seconds (interval);

  CommandLine cmd;

  cmd.AddValue ("packetSize", "Size of application packet sent", packetSize);
  cmd.AddValue ("interval", "Interval (seconds) between packets", interval);
  cmd.AddValue ("generationSize", "Set the generation size to use",
                generationSize);
  //cmd.AddValue ("errorRate", "Packet erasure rate for the links", errorRate);

  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);

  // Set the basic helper for a single link
  PointToPointHelper pointToPoint;

  // Create node containers
  NodeContainer c;
  c.Create (3);
  NodeContainer encoderRecoderNodes = NodeContainer (c.Get (0), c.Get (1));
  NodeContainer recoderDecoderNodes = NodeContainer (c.Get (1), c.Get (2));

  // Create net device containers
  NetDeviceContainer encoderRecoderDevices = pointToPoint.Install (encoderRecoderNodes);
  NetDeviceContainer recoderDecoderDevices = pointToPoint.Install (recoderDecoderNodes);

  // Set IP addresses
  Ipv4AddressHelper ipv4;

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4.Assign (encoderRecoderDevices);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  ipv4.Assign (recoderDecoderDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Set error model for the net devices
  Config::SetDefault ("ns3::RateErrorModel::ErrorUnit",
                      StringValue ("ERROR_UNIT_PACKET"));

  Ptr<RateErrorModel> errorEncoderRecoder = CreateObject<RateErrorModel> ();
  errorEncoderRecoder->SetAttribute ("ErrorRate",
                                     DoubleValue (errorRateEncoderRecoder));

  encoderRecoderDevices->SetAttribute ("ReceiveErrorModel",
                                       PointerValue (errorEncoderRecoder));

  Ptr<RateErrorModel> errorRecoderDecoder = CreateObject<RateErrorModel> ();
  errorRecoderDecoder->SetAttribute ("ErrorRate",
                                     DoubleValue (errorRateRecoderDecoder));

  recoderDecoderDevices->SetAttribute ("ReceiveErrorModel",
                                       PointerValue (errorRecoderDecoder));
  errorEncoderRecoder->Enable ();
  errorRecoderDecoder->Enable ();


  // Creation of RLNC encoder and decoder objects
  rlnc_encoder::factory encoder_factory(generationSize, packetSize);
  rlnc_decoder::factory decoder_factory(generationSize, packetSize);

  // The member build function creates differents instances of each object
  KodoSimulation kodoSimulator(encoder_factory.build(),
                               decoder_factory.build(),
                               decoder_factory.build());

  // Setting up application sockets for receivers and senders
  uint16_t port = 80;
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);

  // Receivers
  Ptr<Socket> recoderSocket = Socket::CreateSocket (c.Get (1), tid);
  recoderSocket->Bind (local);
  recoderSocket->SetRecvCallback (MakeCallback (&KodoSimulation::ReceivePacketRecoder,
                                                &kodoSimulator));

  Ptr<Socket> decoderSocket = Socket::CreateSocket (c.Get (2), tid);
  decoderSocket->Bind (local);
  decoderSocket->SetRecvCallback (MakeCallback (&KodoSimulation::ReceivePacketDecoder,
                                                &kodoSimulator));

  // Sender
  Ptr<Socket> encoderSocket = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"),
                                                port);
  encoderSocket->SetAllowBroadcast (true);
  encoderSocket->Connect (remote);

  // Turn on global static routing so we can actually be routed across the star
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Do pcap tracing on all point-to-point devices on all nodes
  pointToPoint.EnablePcapAll ("multihop");

  Simulator::ScheduleWithContext (encoderSocket->GetNode ()->GetId (),
                                  Seconds (1.0),
                                  &KodoSimulation::GenerateTraffic,
                                  &kodoSimulator, encoder, interPacketInterval);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
