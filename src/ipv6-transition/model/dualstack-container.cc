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

#include "ns3/node-list.h"
#include "ns3/names.h"

#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/log.h"
#include "dualstack-container.h"
#include "ns3/virtual-net-device.h"
#include "ns3/ipv4-address-generator.h"
#include "ns3/internet-module.h"

#include "ns3/csma-net-device.h"
#include "ns3/csma-channel.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"

namespace ns3
{

  NS_LOG_COMPONENT_DEFINE("DualStackContainer");

  DualStackContainer::DualStackContainer ()
  {
  }

  DualStackContainer::Iterator
  DualStackContainer::BeginV6 (void) const
  {
    return m_interfaces.begin ();
  }

  DualStackContainer::Iterator
  DualStackContainer::EndV6 (void) const
  {
    return m_interfaces.end ();
  }

  uint32_t
  DualStackContainer::GetNV6 () const
  {
    return m_interfaces.size ();
  }

  uint32_t
  DualStackContainer::GetInterfaceIndexV6 (uint32_t i) const
  {
    return m_interfaces[i].second;
  }

  DualStackContainer::Iterator
  DualStackContainer::BeginV4 (void) const
  {
    return m_interfaces.begin ();
  }

  DualStackContainer::Iterator
  DualStackContainer::EndV4 (void) const
  {
    return m_interfaces.end ();
  }

  uint32_t
  DualStackContainer::GetNV4 () const
  {
    return m_ipv4interfaces.size ();
  }

  uint32_t
  DualStackContainer::GetInterfaceIndexV4 (uint32_t i) const
  {
    return m_ipv4interfaces[i].second;
  }

  void
  DualStackContainer::Add (std::string ipv6Name, uint32_t interface)
  {
    Ptr<Ipv6> ipv6 = Names::Find<Ipv6> (ipv6Name);
    m_interfaces.push_back (std::make_pair (ipv6, interface));
  }

  void
  DualStackContainer::Add (DualStackContainer& c)
  {
    for (InterfaceVectorV6::const_iterator it = c.m_interfaces.begin ();
	it != c.m_interfaces.end (); it++)
      {
	m_interfaces.push_back (*it);
      }
    for (InterfaceVectorV4::const_iterator it = c.m_ipv4interfaces.begin ();
	it != c.m_ipv4interfaces.end (); it++)
      {
	m_ipv4interfaces.push_back (*it);
      }
  }

  void
  DualStackContainer::Add (Ptr<Ipv6> ipv6, uint32_t interface)
  {
    m_interfaces.push_back (std::make_pair (ipv6, interface));
  }

  void
  DualStackContainer::Add (Ptr<Ipv4> ipv4, uint32_t interface)
  {
    m_ipv4interfaces.push_back (std::make_pair (ipv4, interface));
  }

  Ipv6Address
  DualStackContainer::GetIpv6Address (uint32_t i, uint32_t j) const
  {
    Ptr<Ipv6> ipv6 = m_interfaces[i].first;
    NS_ASSERT_MSG(ipv6,
		  "DualStackContainer::GetIpv6Address(): IP stack not found.");

    uint32_t interface = m_interfaces[i].second;
    return ipv6->GetAddress (interface, j).GetAddress ();
  }
  Ipv4Address
  DualStackContainer::GetIpv4Address (uint32_t i, uint32_t j) const
  {
    Ptr<Ipv4> ipv4 = m_ipv4interfaces[i].first;
    uint32_t interface = m_ipv4interfaces[i].second;
    return ipv4->GetAddress (interface, j).GetLocal ();
  }

  void
  DualStackContainer::Set6to4Forwarding (uint32_t i, bool router)
  {
    if (i < GetNV6 ())
      {
	Ptr<Ipv6> ipv6 = m_interfaces[i].first;
	ipv6->SetForwarding (m_interfaces[i].second, router);
	for (uint32_t count = 0; count <= m_interfaces[i].second;
	    count = count + 1)
	  {
	    ipv6->Set6to4Router (count, true);
	  }
      }
    if (i < GetNV4 ())
      {
	Ptr<Ipv4> ipv4 = m_ipv4interfaces[i].first;
	ipv4->SetForwarding (m_ipv4interfaces[i].second, router);
	for (uint32_t count = 0; count <= m_ipv4interfaces[i].second;
	    count = count + 1)
	  {
	    ipv4->Set6to4Router (count, true);
	  }
      }

  }

  void
  DualStackContainer::SetForwarding (uint32_t i, bool router)
  {
    if (i < GetNV6 ())
      {
	Ptr<Ipv6> ipv6 = m_interfaces[i].first;
	ipv6->SetForwarding (m_interfaces[i].second, router);
      }
    if (i < GetNV4 ())
      {
	Ptr<Ipv4> ipv4 = m_ipv4interfaces[i].first;
	ipv4->SetForwarding (m_ipv4interfaces[i].second, router);

      }

  }
  void
  DualStackContainer::Set6to4Router (uint32_t i, bool Router6to4)
  {

    if (i < GetNV6 ())
      {
	Ptr<Ipv6> ipv6 = m_interfaces[i].first;
	NS_ASSERT_MSG(
	    ipv6,
	    "DualStackContainer::Set6to4Router(): NetDevice is associated" " with a node without IPv6 stack installed -> fail " "(maybe need to use InternetStackHelper?)");

	for (uint32_t count = 0; count <= m_interfaces[i].second;
	    count = count + 1)
	  {
	    ipv6->Set6to4Router (count, Router6to4);
	  }
      }
    if (i < GetNV4 ())
      {
	Ptr<Ipv4> ipv4 = m_ipv4interfaces[i].first;
	NS_ASSERT_MSG(
	    ipv4,
	    "DualStackContainer::Set6to4Router(): NetDevice is associated" " with a node without IPv6 stack installed -> fail " "(maybe need to use InternetStackHelper?)");

	for (uint32_t count = 0; count <= m_ipv4interfaces[i].second;
	    count = count + 1)
	  {
	    ipv4->Set6to4Router (count, Router6to4);
	  }
      }
    Ptr<Ipv6> ipv6 = m_interfaces[i].first;
    NS_ASSERT_MSG(
	ipv6,
	"DualStackContainer::Set6to4Router(): NetDevice is associated" " with a node without IPv6 stack installed -> fail " "(maybe need to use InternetStackHelper?)");

    uint8_t buff[16];
    memset (buff, 0x00, 16);
    buff[0] = 0x20;
    buff[1] = 0x02;
    buff[2] = 0xC0;
    buff[3] = 0x58;
    buff[4] = 0x63;
    buff[5] = 0x01;
    Ipv6Address dest = Ipv6Address (buff);
    //set routing table in 6to4 router
    Ptr<Ipv6StaticRouting> pseudoroute6To4;
    Ipv6StaticRoutingHelper routingHelper;
    pseudoroute6To4 = routingHelper.GetStaticRouting (ipv6);
    pseudoroute6To4->AddNetworkRouteTo (Ipv6Address ("2002::"), Ipv6Prefix (16),
					Ipv6Address ("::"), 1);
    pseudoroute6To4->SetDefaultRoute (dest, 1);

  }
  void
  DualStackContainer::Set6to4Relay (uint32_t router)
  {

    Ptr<Ipv6> ipv6 = m_interfaces[router].first;
    NS_ASSERT_MSG(
	ipv6,
	"DualStackContainer::Set6to4Relay(): NetDevice is associated" " with a node without IPv6 stack installed -> fail " "(maybe need to use InternetStackHelper?)");

    Ptr<Node> node = ipv6->GetObject<Node> ();

    //set routing table in 6to4 relay
    Ptr<Ipv6StaticRouting> pseudoroute6To4;
    Ipv6StaticRoutingHelper routingHelper;
    pseudoroute6To4 = routingHelper.GetStaticRouting (ipv6);
    pseudoroute6To4->AddNetworkRouteTo (Ipv6Address ("2002::"), Ipv6Prefix (16),
					Ipv6Address ("::"), 1);

    Ptr<Ipv4Interface> interface = CreateObject<Ipv4Interface> ();
    Ptr<LoopbackNetDevice> device = CreateObject<LoopbackNetDevice> ();
    node->AddDevice (device);
    interface->SetDevice (device);
    interface->SetNode (node);
    Ipv4InterfaceAddress ifaceAddr = Ipv4InterfaceAddress (
	Ipv4Address ("192.88.99.1"), Ipv4Mask ("255.255.255.0")); // /24 addresses for
    interface->AddAddress (ifaceAddr);
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    NS_ASSERT_MSG(
	ipv4,
	"DualStackContainer::Set6to4Relay(): NetDevice is associated" " with a node without IPv4 stack installed -> fail " "(maybe need to use InternetStackHelper?)");

    int32_t iif = ipv4->GetInterfaceForDevice (device);
    if (iif == -1)
      {
	iif = ipv4->AddInterface (device);
      }
    NS_ASSERT_MSG(
	interface >= 0,
	"DualStackContainer::Set6to4Relay(): " "Interface index not found");

    interface->SetUp ();
    ipv4->AddAddress (iif, ifaceAddr);
    ipv4->SetMetric (iif, 1);
    ipv4->SetUp (iif);

    if (router < GetNV6 ())
      {
	Ptr<Ipv6> ipv6 = m_interfaces[router].first;
	NS_ASSERT_MSG(
	    ipv6,
	    "DualStackContainer::Set6to4Relay(): NetDevice is associated" " with a node without IPv6 stack installed -> fail " "(maybe need to use InternetStackHelper?)");

	for (uint32_t count = 0; count <= m_interfaces[router].second;
	    count = count + 1)
	  {
	    ipv6->Set6to4Router (count, true);
	  }
      }
    if (router < GetNV4 ())
      {
	Ptr<Ipv4> ipv4 = m_ipv4interfaces[router].first;
	NS_ASSERT_MSG(
	    ipv4,
	    "DualStackContainer::Set6to4Relay(): NetDevice is associated" " with a node without IPv4 stack installed -> fail " "(maybe need to use InternetStackHelper?)");

	for (uint32_t count = 0; count <= m_ipv4interfaces[router].second;
	    count = count + 1)
	  {
	    ipv4->Set6to4Router (count, true);
	  }
      }
  }

  void
  DualStackContainer::SetDefaultRoute (uint32_t i, uint32_t router)
  {
    NS_ASSERT_MSG(
	i != router,
	"A node shouldn't set itself as the default router, isn't it? Aborting.");

    Ptr<Ipv6> ipv6 = m_interfaces[i].first;

    Ipv6Address routerAddress = GetLinkLocalAddress (router);
    NS_ASSERT_MSG(routerAddress != Ipv6Address::GetAny (),
		  "No link-local address found on router, aborting");

    Ptr<Ipv6StaticRouting> routing = 0;
    Ipv6StaticRoutingHelper routingHelper;

    routing = routingHelper.GetStaticRouting (ipv6);
    routing->SetDefaultRoute (routerAddress, m_interfaces[i].second);
  }

  void
  DualStackContainer::SetDefaultRouteInAllNodes (uint32_t router)
  {
    if (router < GetNV6 ())
      {
	Ptr<Ipv6> ipv6 = m_interfaces[router].first;
	uint32_t other;

	Ipv6Address routerAddress = GetLinkLocalAddress (router);
	NS_ASSERT_MSG(routerAddress != Ipv6Address::GetAny (),
		      "No link-local address found on router, aborting");

	for (other = 0; other < m_interfaces.size (); other++)
	  {
	    if (other != router)
	      {
		Ptr<Ipv6StaticRouting> routing = 0;
		Ipv6StaticRoutingHelper routingHelper;

		ipv6 = m_interfaces[other].first;
		routing = routingHelper.GetStaticRouting (ipv6);
		routing->SetDefaultRoute (routerAddress,
					  m_interfaces[other].second);
	      }
	  }
      }


  }

  void
  DualStackContainer::SetDefaultRoute (uint32_t i, Ipv6Address routerAddr)
  {
    uint32_t routerIndex = 0;
    bool found = false;
    for (uint32_t index = 0; index < m_interfaces.size (); index++)
      {
	Ptr<Ipv6> ipv6 = m_interfaces[index].first;
	for (uint32_t i = 0;
	    i < ipv6->GetNAddresses (m_interfaces[index].second); i++)
	  {
	    Ipv6Address addr =
		ipv6->GetAddress (m_interfaces[index].second, i).GetAddress ();
	    if (addr == routerAddr)
	      {
		routerIndex = index;
		found = true;
		break;
	      }
	  }
	if (found)
	  {
	    break;
	  }
      }
    NS_ASSERT_MSG(found != true,
		  "No such address in the interfaces. Aborting.");

    NS_ASSERT_MSG(
	i != routerIndex,
	"A node shouldn't set itself as the default router, isn't it? Aborting.");

    Ptr<Ipv6> ipv6 = m_interfaces[i].first;
    Ipv6Address routerLinkLocalAddress = GetLinkLocalAddress (routerIndex);
    Ptr<Ipv6StaticRouting> routing = 0;
    Ipv6StaticRoutingHelper routingHelper;

    routing = routingHelper.GetStaticRouting (ipv6);
    routing->SetDefaultRoute (routerLinkLocalAddress, m_interfaces[i].second);
  }

  Ipv6Address
  DualStackContainer::GetLinkLocalAddress (uint32_t index)
  {
    Ptr<Ipv6> ipv6 = m_interfaces[index].first;
    for (uint32_t i = 0; i < ipv6->GetNAddresses (m_interfaces[index].second);
	i++)
      {
	Ipv6InterfaceAddress iAddress;
	iAddress = ipv6->GetAddress (m_interfaces[index].second, i);
	if (iAddress.GetScope () == Ipv6InterfaceAddress::LINKLOCAL)
	  {
	    return iAddress.GetAddress ();
	  }
      }
    return Ipv6Address::GetAny ();
  }

}/* namespace ns3 */
