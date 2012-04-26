#ifndef PEP_WIFI_NET_DEVICE_H
#define PEP_WIFI_NET_DEVICE_H


#include <ns3/wifi-net-device.h>
#include <ns3/packet.h>
#include <ns3/adhoc-wifi-mac.h>
#include <kodo/rlnc/full_vector_codes.h>
#include "code-header.h"

// @todo: not in ns3 namespace
namespace ns3 {


// class PepWifiNetDevice : public ns3::WifiNetDevice
class PepWifiNetDevice : public WifiNetDevice
{
public:
  static TypeId GetTypeId (void);

  PepWifiNetDevice ();
  ~PepWifiNetDevice ();

  typedef kodo::full_rlnc_encoder<fifi::binary8> rlnc_encoder;
  typedef kodo::full_rlnc_decoder<fifi::binary8> rlnc_decoder;



  std::map<int, rlnc_decoder::pointer> forward;
  std::map<int, rlnc_decoder::pointer> decoding;
  std::map<int, int> decoded_flag;

  // @todo: int m_code (for all member varibles)

  bool m_configComplete;
  int code;
  int recode;
  int sent_packet;
  double interval;
  int generation;
  int received;
  int countcode;
  int max_symbols;
  int max_size;
  int from_source;
  int from_relay;
  int rank;
  int inc;
  int ninc;
  int rsource;
  int r;
  int temp;
  int sent_code;
  rlnc_encoder::factory m_encoder_factory;
  rlnc_decoder::factory m_decoder_factory;
  rlnc_encoder::pointer encoder;
  std::vector<uint8_t> payload;
  int relay_activity;
  int seed;
  int received_relay;
  // @todo when you move out of ns3 namespace use
  // struct coded : public ns3::Object
  struct coded : public Object
  {
    int k;
    double t2;

    rlnc_encoder::pointer m_encoder;
    std::vector<uint8_t> m_encoder_data;

    uint16_t protocolNumber;
    CodeHeader h1;
    Mac48Address realTo;
  };

  std::list<Ptr<Packet> > m_queue;

  struct Item
  {
    Item (Ptr< Packet> packet)
      : m_packet (packet)
    {
    }

    Ptr<Packet> m_packet;
  };


  void Enqueue1 (Ptr<Packet> packet);

  // PacketQueue m_queue;

public:
  // From WifiNetDevice
  virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);

public:
  Ptr<Packet> rencoding (Ptr<Packet> packet,int seq);
  bool DecodingReceive (Ptr< NetDevice > device, Ptr< const Packet > packet, uint16_t type, const Address & from);
  bool ReceivedSource (const Address & from, Mac48Address des, Ptr< const Packet > packet);
  bool ReceivedSink (Mac48Address source, Ptr< const Packet > packet, const Address & from, uint16_t type, Ptr<AdhocWifiMac> m_mac);
  bool ReceivedRelay (Mac48Address des, uint16_t type, Ptr< const Packet > packet);
  virtual bool coding (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
  void SendCode (Ptr <coded> m_coded);
  
  // The ns3 function which handle incomming packets
  NetDevice::ReceiveCallback m_receiveCallback;



};

}

#endif
