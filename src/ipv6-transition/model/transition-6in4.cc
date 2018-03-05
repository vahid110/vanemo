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

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "transition-6in4.h"

namespace ns3
{

  NS_LOG_COMPONENT_DEFINE("Transition6In4");

  Transition6In4::Transition6In4 ()
  {
		NS_LOG_FUNCTION_NOARGS();
  }

  void
  Transition6In4::Set6In4 (Ptr<Node> tunnelSource, Ptr<Node> tunnelDestination,
			   std::string sourceNetwork,
			   std::string destinationNetwork,
			   Ipv4Address sourceAddress, Ipv4Address destAddress)
  {
		NS_LOG_FUNCTION_NOARGS();

    Ptr<Ipv6> source = tunnelSource->GetObject<Ipv6> ();
    NS_ASSERT_MSG(source, "Transition6In4::Set6In4: Bad ipv6");

    Ptr<Ipv4> v4Destination = tunnelDestination->GetObject<Ipv4> ();
    NS_ASSERT_MSG(v4Destination, "Transition6In4::Set6In4: Bad ipv4");

    uint32_t interfaceDest = v4Destination->GetInterfaceForAddress (
	destAddress);
    Ptr<Ipv6StaticRouting> routing = 0;
    Ipv6StaticRoutingHelper routingHelper;
    routing = routingHelper.GetStaticRouting (source);
    uint32_t ip4 = destAddress.Get ();
    uint8_t buff[16];
    memset (buff, 0x00, 16);
    buff[0] = 0xa4;
    buff[12] = ((ip4 >> 24) & 0xff);
    buff[13] = ((ip4 >> 16) & 0xff);
    buff[14] = ((ip4 >> 8) & 0xff);
    buff[15] = ((ip4 >> 0) & 0xff);

    Ipv6Address gateway = Ipv6Address (buff);

    routing->AddNetworkRouteTo (Ipv6Address (destinationNetwork.c_str ()),
				Ipv6Prefix (64), gateway, interfaceDest);
    Ptr<Ipv6> destination = tunnelDestination->GetObject<Ipv6> ();
    Ptr<Ipv4> v4Source = tunnelSource->GetObject<Ipv4> ();
    uint32_t interfaceSource = v4Source->GetInterfaceForAddress (sourceAddress);
    routing = 0;
    routing = routingHelper.GetStaticRouting (destination);

    ip4 = 0;
    ip4 = sourceAddress.Get ();
    uint8_t buff2[16];
    memset (buff2, 0x00, 16);
    buff2[0] = 0xa4;
    buff2[12] = ((ip4 >> 24) & 0xff);
    buff2[13] = ((ip4 >> 16) & 0xff);
    buff2[14] = ((ip4 >> 8) & 0xff);
    buff2[15] = ((ip4 >> 0) & 0xff);

    Ipv6Address gateway2 = Ipv6Address (buff2);
    routing->AddNetworkRouteTo (Ipv6Address (sourceNetwork.c_str ()),
				Ipv6Prefix (64), gateway2, interfaceSource);
  }

  bool
  Transition6In4::Is6In4Gateway (Ipv6Address gateway)
  {
		NS_LOG_FUNCTION_NOARGS();
    uint8_t buff[16];
    gateway.GetBytes (buff);

    NS_LOG_FUNCTION(gateway);
    NS_LOG_LOGIC("Checking if gateway is 6in4 tunnel: " << gateway);
    uint8_t v4MappedPrefix[12] =
      { 0xa4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    if (memcmp (buff, v4MappedPrefix, sizeof(v4MappedPrefix)) == 0)
      {
	return (true);
      }
    return (false);
  }

}
