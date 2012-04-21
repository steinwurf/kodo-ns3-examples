#include "code-header.h"
#include <ns3/address-utils.h>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (CodeHeader);

CodeHeader::CodeHeader ()
  : m_destinationPort (0)
{
}
CodeHeader::~CodeHeader ()
{

  m_destinationPort = 0;

}


TypeId
CodeHeader::GetTypeId (void)
{
  static TypeId tid = 
  tid = TypeId ("CodeHeader")
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
  return 2;
}
void
CodeHeader::Serialize (Buffer::Iterator start) const
{
  // The 2 byte-constant
  start.WriteHtonU16 (m_destinationPort);



}
uint32_t
CodeHeader::Deserialize (Buffer::Iterator start)
{
  m_destinationPort = start.ReadNtohU16 ();
  return 2;    // the number of bytes consumed.
}
void
CodeHeader::Print (std::ostream &os) const
{
  os << m_destinationPort;
}

void
CodeHeader::SetGeneration (uint16_t port)
{
  m_destinationPort = port;
}

uint16_t
CodeHeader::GetGeneration (void) const
{
  return m_destinationPort;
}


}
