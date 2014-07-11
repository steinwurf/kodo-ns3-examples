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
// The code below is based on the wifi-simple-adhoc example, which can
// be found here ns-3-dev/examples/wireless/wifi-simple-adhoc.cc in the
// ns-3 source code.

// In the script below the sender transmits encoded packets in a non-systematic
// way from a block of data. The sender continues until the receiver has all
// the packets. The description below is from the original example, we modified
// it at bit fit our scenario.

// This script configures two nodes on an 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one generation of
// 5 packets and 1000 (application) bytes to the other node. The physical
// layer is configured to receive at a fixed RSS (regardless of the distance
// and transmit power); therefore, changing position of the nodes has no effect.
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc --help"
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when rss drops below -96 dBm.
// To see this effect, try by changing the rss parameter on the simulation
// by typing ./build/linux/wifi_broadcast/wifi_broadcast --rss=-96
// With this value (or higher), the erasure rate goes to 1 and the packets can
// not be recovered.
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// When you are done, you will notice two pcap trace files in your directory.
// You can review the files with Wireshark or tcpdump. If you have tcpdump
// installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-0-0.pcap -nn -tt

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/config-store-module.h>
#include <ns3/wifi-module.h>
#include <ns3/internet-module.h>

#include <kodo/rlnc/full_rlnc_codes.hpp>
#include <kodo/trace.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>

using namespace ns3;

// The encoder / decoder type we will use

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
                 const rlnc_decoder::pointer& decoder)
    : m_encoder(encoder),
      m_decoder(decoder)
  {

    // Initialize the encoder with some random data
    std::vector<uint8_t> data(encoder->block_size(), 'x');
    m_encoder->set_symbols(sak::storage(data));

    m_encoder->set_systematic_off();
    m_encoder->seed(time(0));

    m_payload_buffer.resize(m_encoder->payload_size());
    m_transmission_count = 0;
  }

  void ReceivePacket (Ptr<Socket> socket)
  {

    std::cout << "Received one packet at decoder" << std::endl;

    auto packet = socket->Recv();
    packet->CopyData(&m_payload_buffer[0], m_decoder->payload_size());

    m_decoder->decode(&m_payload_buffer[0]);

    if (kodo::has_trace<rlnc_decoder>::value)
    {
        auto filter = [](const std::string& zone)
        {
            std::set<std::string> filters =
                {"input_symbol_coefficients","decoder_state"};

            return filters.count(zone);
        };

        std::cout << "Trace decoder:" << std::endl;
        kodo::trace(m_decoder, std::cout, filter);
    }

  }

  void GenerateTraffic (Ptr<Socket> socket, Time pktInterval )
  {
    if(!m_decoder->is_complete())
      {
        uint32_t bytes_used = m_encoder->encode(&m_payload_buffer[0]);
        auto packet = Create<Packet> (&m_payload_buffer[0],
                                      bytes_used);
        socket->Send (packet);
        m_transmission_count++;

        if (kodo::has_trace<rlnc_encoder>::value)
        {
            std::cout << "Trace encoder:" << std::endl;
            kodo::trace(m_encoder, std::cout);
        }

        Simulator::Schedule (pktInterval, &KodoSimulation::GenerateTraffic, this,
                             socket, pktInterval);

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
  rlnc_decoder::pointer m_decoder;

  std::vector<uint8_t> m_payload_buffer;

  uint32_t m_transmission_count;

};


int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double rss = -93;  // -dBm
  uint32_t packetSize = 1000; // bytes
  double interval = 1.0; // seconds
  uint32_t generationSize = 5;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("generationSize", "Set the generation size to use",
                generationSize);

  cmd.Parse (argc, argv);

  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
                      StringValue ("2200"));

  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                      StringValue ("2200"));

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  // Source and destination
  NodeContainer c;
  c.Create (2);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",
                                  DoubleValue (rss));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength. However, they are required for the
  // YansWiFiChannelHelper
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc =
    CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  rlnc_encoder::factory encoder_factory(generationSize, packetSize);
  rlnc_decoder::factory decoder_factory(generationSize, packetSize);

  KodoSimulation kodoSimulator(encoder_factory.build(),
                               decoder_factory.build());

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&KodoSimulation::ReceivePacket,
                                           &kodoSimulator));

  Ptr<Socket> source = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"),
                                                80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Pcap tracing
  wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0),
                                  &KodoSimulation::GenerateTraffic,
                                  &kodoSimulator,
                                  source, interPacketInterval);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

