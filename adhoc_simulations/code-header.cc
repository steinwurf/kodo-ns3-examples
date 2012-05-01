#include "code-header.h"
#include <ns3/address-utils.h>

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (CodeHeader);

CodeHeader::CodeHeader ()
  : m_generation (0)
{
}
CodeHeader::~CodeHeader ()
{

  m_generation = 0;

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
  return 2;
}
void
CodeHeader::Serialize (Buffer::Iterator start) const
{
  // The 2 byte-constant
  start.WriteHtonU16 (m_generation);



}
uint32_t
CodeHeader::Deserialize (Buffer::Iterator start)
{
  m_generation = start.ReadNtohU16 ();
  return 2; // the number of bytes consumed.
}
void
CodeHeader::Print (std::ostream &os) const
{
  os << m_generation;
}

void
CodeHeader::SetGeneration (uint16_t gen)
{
  m_generation = gen;
}

void
CodeHeader::SetMacSource ( Mac48Address source)
{
  m_Macsource = source;
}

void
CodeHeader::SetMacSink ( Mac48Address sink)
{
  m_Macsink = sink;
}

Mac48Address
CodeHeader::GetMacSource (void) const
{
  return m_Macsource;
}

Mac48Address
CodeHeader::GetMacSink (void) const
{
  return m_Macsink;
}

uint16_t
CodeHeader::GetGeneration (void) const
{
  return m_generation;
}


}
