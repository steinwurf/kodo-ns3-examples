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
// within a ns-3 simulation. The code below is based on the
// udp_simple_broadcast example, which can be found in this repository.

// In the script below the sender transmits encoded packets from a block of
// data to two receivers. The sender continues until all receivers have received
// all packets. Here the packets are sent using the binary field, GF(2)  with
// a generation of 5 packets and 1000 (application) bytes to the other node.

// You can change the generation size or another parameter, by running (for
// example with a different generation size):
// ./build/linux/wired_broadcast/wired_broadcast --generationSize=GENERATION_SIZE

// When you are done, you will notice four pcap trace files in your directory.
// You can review the files with Wireshark or tcpdump. If you have tcpdump
// installed, you can try this:
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
                 const rlnc_decoder::pointer& decoder1,
                 const rlnc_decoder::pointer& decoder2)
    : m_encoder(encoder),
      m_decoder_1(decoder1),
      m_decoder_2(decoder2)
  {
    m_encoder->set_systematic_off();
    m_encoder->seed(time(0));

    // Initialize the input data
    std::vector<uint8_t> data(encoder->block_size(), 'x');
    m_encoder->set_symbols(sak::storage(data));

    m_payload_buffer.resize(m_encoder->payload_size());
    m_transmission_count = 0;
  }

  void ReceivePacket1 (Ptr<Socket> socket)
  {
    auto packet = socket->Recv();
    packet->CopyData(&m_payload_buffer[0], m_decoder_1->payload_size());
    m_decoder_1->decode(&m_payload_buffer[0]);
    std::cout << "Received one packet at decoder 1" << std::endl;

    if (kodo::has_trace<rlnc_decoder>::value)
    {
      auto filter = [](const std::string& zone)
      {
        std::set<std::string> filters =
          {"decoder_state","input_symbol_coefficients"};
        return filters.count(zone);
      };

      std::cout << "Trace decoder 1:" << std::endl;
      kodo::trace(m_decoder_1, std::cout, filter);
    }
  }

  void ReceivePacket2 (Ptr<Socket> socket)
  {
    auto packet = socket->Recv();
    packet->CopyData(&m_payload_buffer[0], m_decoder_2->payload_size());
    m_decoder_2->decode(&m_payload_buffer[0]);
    std::cout << "Received one packet at decoder 2" << std::endl;

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
    if (!m_decoder_1->is_complete() || !m_decoder_2->is_complete())
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
  rlnc_decoder::pointer m_decoder_1;
  rlnc_decoder::pointer m_decoder_2;

  std::vector<uint8_t> m_payload_buffer;
  uint32_t m_transmission_count;
};

int main (int argc, char *argv[])
{
  uint32_t packetSize = 1000; // Application bytes per packet
  double interval = 1.0; // Time between events
  uint32_t generationSize = 5; // RLNC generation size
  double errorRate = 0.3; // Error rate for all the links

  Time interPacketInterval = Seconds (interval);

  CommandLine cmd;

  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("generationSize", "Set the generation size to use",
                generationSize);
  cmd.AddValue ("errorRate", "Packet erasure rate for the links", errorRate);

  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);

  // Set the basic helper for a single link
  PointToPointHelper pointToPoint;

  // Two receivers against a centralized hub. Note: DO NOT CHANGE THIS LINE
  PointToPointStarHelper star (2,pointToPoint);

  // Set error model for the net devices
  Config::SetDefault ("ns3::RateErrorModel::ErrorUnit",
                      StringValue ("ERROR_UNIT_PACKET"));

  Ptr<RateErrorModel> error_model = CreateObject<RateErrorModel> ();
  error_model->SetAttribute ("ErrorRate", DoubleValue (errorRate));

  star.GetSpokeNode(0)->GetDevice(0)->SetAttribute ("ReceiveErrorModel",
                                                    PointerValue (error_model));
  star.GetSpokeNode(1)->GetDevice(0)->SetAttribute ("ReceiveErrorModel",
                                                    PointerValue (error_model));
  error_model->Enable ();

  // Setting IP protocol stack
  InternetStackHelper internet;
  star.InstallStack(internet);

  // Set IP addresses
  star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));

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
  Ptr<Socket> recvSink1 = Socket::CreateSocket (star.GetSpokeNode(0), tid);
  recvSink1->Bind (local);
  recvSink1->SetRecvCallback (MakeCallback (&KodoSimulation::ReceivePacket1,
                                            &kodoSimulator));

  Ptr<Socket> recvSink2 = Socket::CreateSocket (star.GetSpokeNode(1), tid);
  recvSink2->Bind (local);
  recvSink2->SetRecvCallback (MakeCallback (&KodoSimulation::ReceivePacket2,
                                            &kodoSimulator));

  // Sender
  Ptr<Socket> source = Socket::CreateSocket (star.GetHub(), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"),
                                                port);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Turn on global static routing so we can actually be routed across the star
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Do pcap tracing on all point-to-point devices on all nodes
  pointToPoint.EnablePcapAll ("star");

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (), Seconds (1.0),
                                  &KodoSimulation::GenerateTraffic,
                                  &kodoSimulator, source, interPacketInterval);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
