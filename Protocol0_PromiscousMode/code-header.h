#ifndef CODE_HEADER_H
#define CODE_HEADER_H

#include <stdint.h>
#include <string>
#include <ns3/header.h>
#include <ns3/ipv4-address.h>
#include <ns3/ipv6-address.h>
#include <ns3/adhoc-wifi-mac.h>
namespace ns3 {

class CodeHeader : public Header
{
public:
  CodeHeader ();
  ~CodeHeader ();
  void SetGeneration (uint16_t port);
  uint16_t GetGeneration (void) const;
  void EnableCode (void);
  void DisableCode (void);
  uint16_t GetCode (void) ;
  void SetDestinationIp (Ipv4Address destination);
  Ipv4Address GetDestinationIp (void); 
   void SetSourceMAC (Mac48Address destination);
    Mac48Address GetSourceMAC (void) ;
    
  // must be implemented to become a valid new header.
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;
  // allow protocol-specific access to the header data.

private:
  uint16_t m_generation;
  uint16_t m_code;
  Mac48Address m_source_mac;
  Ipv4Address m_destination_ip;
};
}
#endif
