/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * Copyright (c) 2014 FIIT STU
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
 * Author: Lukáš Danielovic <lukas.danielovic@gmail.com>
 */

#include "ns3/assert.h"
#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/ipv4-header.h"

#include "ipv6-encapsulation-header.h"

NS_LOG_COMPONENT_DEFINE("Ipv6EncapsulationHeader");

namespace ns3
{

  NS_OBJECT_ENSURE_REGISTERED(Ipv6EncapsulationHeader);

  Ipv6EncapsulationHeader::Ipv6EncapsulationHeader ()
  {
		NS_LOG_FUNCTION_NOARGS();
    m_size = 0;
  }
  Ipv6EncapsulationHeader::~Ipv6EncapsulationHeader ()
  {
		NS_LOG_FUNCTION_NOARGS();

  }

  TypeId
  Ipv6EncapsulationHeader::GetTypeId (void)
  {
		NS_LOG_FUNCTION_NOARGS();
    static TypeId tid =
	TypeId ("ns3::Ipv6EncapsulationHeader").SetParent<Header> ().AddConstructor<
	    Ipv6EncapsulationHeader> ();
    return tid;
  }
  TypeId
  Ipv6EncapsulationHeader::GetInstanceTypeId (void) const
  {
		NS_LOG_FUNCTION_NOARGS();
    return GetTypeId ();
  }

  void
  Ipv6EncapsulationHeader::Print (std::ostream &os) const
  {
		NS_LOG_FUNCTION_NOARGS();
    os << "data=" << m_packet;
  }
  uint32_t
  Ipv6EncapsulationHeader::GetSerializedSize (void) const
  {

    return m_size;
  }
  void
  Ipv6EncapsulationHeader::Serialize (Buffer::Iterator start) const
  {
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    uint32_t size = m_packet->GetSize ();
    uint8_t *buf = new uint8_t[size];
    m_packet->CopyData (buf, size);
    i.Write (buf, size);
    delete[] buf;
  }
  uint32_t
  Ipv6EncapsulationHeader::Deserialize (Buffer::Iterator start)
  {
    NS_LOG_FUNCTION(this << &start);
    uint16_t length = start.GetSize ();
    uint8_t* data = new uint8_t[length];
    Buffer::Iterator i = start;

    i.Read (data, length);
    m_packet = Create<Packet> (data, length);

    delete[] data;
    return GetSerializedSize ();

  }

  void
  Ipv6EncapsulationHeader::SetPacket (Ptr<Packet> p)
  {
    NS_LOG_FUNCTION(this << m_packet);

    NS_LOG_FUNCTION(this << *p);
    m_packet = p;
    m_size = m_packet->GetSize ();

  }
  Ptr<Packet>
  Ipv6EncapsulationHeader::GetPacket () const
  {
    NS_LOG_FUNCTION(this << m_packet);

    return m_packet;
  }

}
