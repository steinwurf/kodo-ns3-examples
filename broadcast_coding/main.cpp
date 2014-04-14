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

// This example shows how to use the Kodo library in a ns-3 simulation.
// The code below are based the the wifi-simple-adhoc example, which can
// be found here ns-3-dev/examples/wireless/wifi-simple-adhoc.cc in the
// ns-3 source code.
// In the script below the sender transmits encoded packets from a block of
// data. The sender continues until the receiver has all packets. The
// description below is from the original example, we modified it at bit
// fit our scenario.

// This script configures two nodes on an 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one generation of
// 32 packets and 1000 (application) bytes to the other node.  The physical
// layer is configured to receive at a fixed RSS (regardless of the distance
// and transmit power); therefore, changing position of the nodes has no effect.
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc --help"
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when rss drops below -97 dBm.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --rss=-97 --generationSize=20"
// ./waf --run "wifi-simple-adhoc --rss=-98 --generationSize=20"
// ./waf --run "wifi-simple-adhoc --rss=-99 --generationSize=20"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
//
// ./waf --run "wifi-simple-adhoc --verbose=1"
//
// When you are done, you will notice two pcap trace files in your directory.
// If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-0-0.pcap -nn -tt
//
// TO BE EDITED

#include <ns3/core-module.h>
#include <point-to-point-star.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/config-store-module.h>
#include <ns3/wifi-module.h>
#include <ns3/internet-module.h>

#include <kodo/rlnc/full_vector_codes.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

NS_LOG_COMPONENT_DEFINE ("KodoWifiCentralizedCodedBroadcast");

using namespace ns3;

// The encoder / decoder type we will use
typedef kodo::full_rlnc_encoder<fifi::binary> rlnc_encoder;
typedef kodo::full_rlnc_decoder<fifi::binary> rlnc_decoder;

// Just for illustration purposes, this simple objects implements both
// the sender (encoder) and receiver (decoder).
class KodoSimulation
{
public:

  KodoSimulation(const rlnc_encoder::pointer& encoder,
                 const rlnc_decoder::pointer& decoder)
    : m_encoder(encoder),
      m_decoder(decoder)
  {

    // Initialize the encoder with some random data
    std::vector<uint8_t> data(encoder->block_size(), 'x');
    m_encoder->set_symbols(sak::storage(data));

    m_payload_buffer.resize(m_encoder->payload_size());
  }

  void ReceivePacket (Ptr<Socket> socket)
  {
    NS_LOG_UNCOND ("Received one packet!");

    auto packet = socket->Recv();
    packet->CopyData(&m_payload_buffer[0], m_decoder->payload_size());

    m_decoder->decode(&m_payload_buffer[0]);

  }

  void GenerateTraffic (Ptr<Socket> socket, Time pktInterval )
  {
    if(!m_decoder->is_complete())
      {
        uint32_t bytes_used = m_encoder->encode(&m_payload_buffer[0]);
        auto packet = Create<Packet> (&m_payload_buffer[0],
                                      bytes_used);
        socket->Send (packet);
        Simulator::Schedule (pktInterval, &KodoSimulation::GenerateTraffic, this,
                             socket, pktInterval);
      }
    else
      {
        socket->Close ();
      }
  }

private:

  rlnc_encoder::pointer m_encoder;
  rlnc_decoder::pointer m_decoder;

  std::vector<uint8_t> m_payload_buffer;

};

void set_client(Ptr<Node> client, uint16_t port)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (client, tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&KodoSimulation::ReceivePacket,
                                           &kodoSimulator));
}

int main (int argc, char *argv[])
{

  /*std::string phyMode ("DsssRate1Mbps");
  double rss = -80;  // -dBm
  uint32_t packetSize = 1000; // bytes
  double interval = 1.0; // seconds
  bool verbose = false;
  uint32_t generationSize = 32;
  float erasure = 0

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("generationSize", "Set the generation size to use", generationSize);

  cmd.Parse (argc, argv);
  */

  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // Attributes of each link against the hub (homogeneous links)
  NS_LOG_INFO ("Defining link topology...");
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  // Two clients against a centralized hub
  NS_LOG_INFO ("Creating star topology...");
  PointToPointStarHelper pointToPointStar (2,pointToPoint);

  // Set IP addresses
  NS_LOG_INFO ("Assigning IP Addresses...");
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  pointToPointStar.AssignIpv4Addresses(address);

  // Setting IP protocol stack
  NS_LOG_INFO ("Setting IP protocol...");
  InternetStackHelper internet;
  pointToPointStar.InstallStack(internet);

  // Creation of RLNC encoder and decoder objects
  rlnc_encoder::factory encoder_factory(generationSize, packetSize);
  rlnc_decoder::factory decoder_factory(generationSize, packetSize);

  KodoSimulation kodoSimulator(encoder_factory.build(),
                               decoder_factory.build());

  // Setting up application sockets for clients and server
  NS_LOG_INFO ("Setting UDP Sockets...");
  uint16_t port = 80;

  set_client(pointToPointStar.GetSpokeNode(0),port);
  set_client(pointToPointStar.GetSpokeNode(1),port);

  Ptr<Socket> source = Socket::CreateSocket (pointToPointStar.GetHub(), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), port);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Tracing
  //wifiPhy.EnablePcap ("wifi-simple-adhoc", devices); //Check with point multipoint

  // Output what we are doing
  //NS_LOG_UNCOND ("Testing " << generationSize  << " packets sent "
  //               << "with receiver rss " << rss );

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0), &KodoSimulation::GenerateTraffic,
                                  &kodoSimulator,
                                  source, interPacketInterval);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
