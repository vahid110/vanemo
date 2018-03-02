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

#ifndef IPV6_ENCAPSULATION_HEADER_H_
#define IPV6_ENCAPSULATION_HEADER_H_

#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include <iostream>

namespace ns3
{

  class Packet;
  /* A sample Header implementation
   */
  class Ipv6EncapsulationHeader : public Header
  {
  public:

    Ipv6EncapsulationHeader ();
    virtual
    ~Ipv6EncapsulationHeader ();

    void
    SetPacket (Ptr<Packet> p);
    Ptr<Packet>
    GetPacket () const;
    static TypeId
    GetTypeId (void);
    virtual TypeId
    GetInstanceTypeId (void) const;
    virtual void
    Print (std::ostream &os) const;
    virtual void
    Serialize (Buffer::Iterator start) const;
    virtual uint32_t
    Deserialize (Buffer::Iterator start);
    virtual uint32_t
    GetSerializedSize (void) const;
  private:
    Ptr<Packet> m_packet;
    uint32_t m_size;

  };

} // namespace ns3

#endif /* IPV6_ENCAPSULATION_HEADER_H_ */
