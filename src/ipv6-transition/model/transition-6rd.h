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

#ifndef TRANSITION_6RD_H
#define TRANSITION_6RD_H

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

namespace ns3
{

  class Transition6Rd
  {
  public:

    /**
     * \brief contructor
     */
    Transition6Rd ();

    /**
     * \brief set ISP network for easier router setting
     * \param ispNetwork IPv6 network address
     * \param prefix IPv6 prefix
     * \param ipv4Prefix IPv4 ISP prefix
     * \param bRAddress anycast address for BR
     */
    void
    SetIsp (std::string ispNetwork, Ipv6Prefix prefix, uint8_t ipv4Prefix,
	    Ipv4Address bRAddress);

    /**
     * \brief creates IPv6 address out of ISP address + IPv4 address + subnet ID < 64
     * \param nodeAddress address of router to be set
     * \param subnetId ID of subnet
     * \param subLenght lenght of subnet ID
     * \returns IPv6 network
     */
    Ipv6Address
    Get6rdNetwork (Ipv4Address nodeAddress, Ipv6Address subnetId,
		   uint32_t subLength);

    /**
     * \brief set 6rd BR from known ISP network configuration
     * \param cE node to be set
     */
    void
    SetBorderRelay (Ptr<Node> cE);

    /**
     * \brief set 6rd CE from known ISP network configuration
     * \param outgoingInterface address of interface that will be IPv4 destination to IPv6 network
     * \param cE node to be set
     */
    void
    SetCustomerEdge (Ipv4Address outgoingInterface, Ptr<Node> cE);

    /**
     * \brief get IPv4 destination to node out of IPv6 destination address
     * \param ispPrefix IPv6 prefix of ISP network
     * \param destination address where is IPv4 address included
     * \param v4Subnet IPv4 prefix of ISP network
     * \param interfaceAddress IPv4 address of node
     * \returns IPv4 destination address
     */
    uint32_t
    GetIpv4Dest (Ipv6Prefix ispPrefix, Ipv6Address destination,
		 uint8_t v4Subnet, Ipv4Address interfaceAddress);

  private:

    Ipv6InterfaceAddress m_ispNetwork;
    uint8_t m_ipv4Prefix;
    uint32_t m_bRAddress;
    static const uint32_t IPV6_BITS = 128;
    uint32_t shift[IPV6_BITS];

    /**
     * \brief helper to get prefix number
     */
    uint32_t
    PrefixToIndex (Ipv6Prefix prefix) const;

    /**
     * \brief get bit number
     */
    uint8_t
    getBit (uint32_t shift);

  };

}

#endif
