#include "code-header.h"
#include <ns3/address-utils.h>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (CodeHeader);

CodeHeader::CodeHeader ()
  : m_generation (0),
  m_code(1)

{
	
}
CodeHeader::~CodeHeader ()
{

  m_generation = 0;
  m_code = 0;

}


TypeId
CodeHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("CodeHeader")
    .SetParent<Header> ()
    .AddConstructor<CodeHeader> ()
  ;
  return tid;
}
TypeId
CodeHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
CodeHeader::GetSerializedSize (void) const
{
  return 14;
}
void
CodeHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU16 (m_generation);
  i.WriteHtonU16 (m_code);
  WriteTo (i, m_source_mac);
  WriteTo (i, m_destination_ip);
}
uint32_t
CodeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_generation = i.ReadNtohU16 ();
  m_code=i.ReadNtohU16 ();
  ReadFrom (i, m_source_mac);
  ReadFrom (i, m_destination_ip);
  return 14; // the number of bytes consumed.
}
void
CodeHeader::Print (std::ostream &os) const
{
  os << m_generation;
  os << m_code ;
  os << m_source_mac;
  os << m_destination_ip;
}

void
CodeHeader::SetGeneration (uint16_t generation)
{
  m_generation = generation;
}

uint16_t
CodeHeader::GetGeneration (void) const
{
  return m_generation;
}

void CodeHeader::EnableCode (void) 
{
m_code=1;
}

void CodeHeader::DisableCode (void) 
{
m_code=0;
}
uint16_t CodeHeader::GetCode (void) 
{
return m_code;
}

Mac48Address CodeHeader::GetSourceMAC (void) 
{
return m_source_mac;
}




Ipv4Address CodeHeader::GetDestinationIp (void) 
{
return m_destination_ip;
}

void CodeHeader::SetSourceMAC (Mac48Address source) 
{
 m_source_mac=source;
}

void CodeHeader::SetDestinationIp (Ipv4Address destination) 
{
 m_destination_ip=destination;
}



}
