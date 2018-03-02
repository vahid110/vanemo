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

#include "ns3/internet-module.h"
#include "transition-6rd.h"

namespace ns3
{

  NS_LOG_COMPONENT_DEFINE("Transition6Rd");

  Transition6Rd::Transition6Rd ()
  {
    m_ispNetwork = Ipv6InterfaceAddress (Ipv6Address ("1234:56::"),
					 Ipv6Prefix (24));
    m_ipv4Prefix = 8;
    for (uint32_t i = 0; i < IPV6_BITS; ++i)
      {
	shift[i] = IPV6_BITS - i;
      }
    m_bRAddress = 0;
  }

  void
  Transition6Rd::SetIsp (std::string ispNetwork, Ipv6Prefix prefix,
			 uint8_t ipv4Prefix, Ipv4Address bRAddress)
  {
    NS_LOG_FUNCTION(this << ispNetwork << prefix << ipv4Prefix << bRAddress);
    Ipv6Address netCheck = Ipv6Address (ispNetwork.c_str ());

    uint32_t index = PrefixToIndex (prefix);
    uint32_t a = index / 8;
    uint32_t b = index % 8;
    uint8_t subnet[16];
    uint8_t nw[16];
    memset (subnet, 0x00, 16);
    memset (nw, 0x00, 16);
    netCheck.GetBytes (subnet);
    NS_LOG_DEBUG("Index " << index);
    for (uint32_t j = 0; j < a; j++)
      {
	nw[j] = subnet[j];
      }

    nw[a] = subnet[a] & getBit (b);

    m_ispNetwork = Ipv6InterfaceAddress (Ipv6Address (nw), prefix);
    NS_ASSERT_MSG(ipv4Prefix >= 0, "Not prefix: lesser than 0");
    NS_ASSERT_MSG(ipv4Prefix <= 32, "Not prefix: bigger than 32");
    m_ipv4Prefix = ipv4Prefix;
    m_bRAddress = bRAddress.Get ();
  }

  void
  Transition6Rd::SetBorderRelay (Ptr<Node> cE)
  {

    Ptr<Ipv6L3Protocol> l3 = cE->GetObject<Ipv6L3Protocol> ();
    l3->Add6RdNetwork (m_ipv4Prefix, m_ispNetwork, m_bRAddress);
    Ipv4Address bRAddress = Ipv4Address (m_bRAddress);
    Ptr<Ipv4> ipv4 = cE->GetObject<Ipv4> ();
    Ptr<Ipv4Interface> interface = CreateObject<Ipv4Interface> ();
    Ptr<LoopbackNetDevice> device = CreateObject<LoopbackNetDevice> ();
    cE->AddDevice (device);
    interface->SetDevice (device);
    interface->SetNode (cE);
    Ipv4InterfaceAddress ifaceAddr = Ipv4InterfaceAddress (
	bRAddress, Ipv4Mask ("255.255.255.0"));
    interface->AddAddress (ifaceAddr);
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

    Ptr<Ipv6StaticRouting> pseudoroute;
    Ipv6StaticRoutingHelper routingHelper;
    Ptr<Ipv6> ipv6 = cE->GetObject<Ipv6> ();
    pseudoroute = routingHelper.GetStaticRouting (ipv6);
    pseudoroute->AddNetworkRouteTo (m_ispNetwork.GetAddress (),
				    m_ispNetwork.GetPrefix (), 1, 1);

    //get destination BR anycast without subnetID
//    Ipv6Address hop = Get6rdNetwork (bRAddress, Ipv6Address ("::"), 0);
//    pseudoroute->AddNetworkRouteTo (hop, Ipv6Prefix (64), 1, 1);

  }
  Ipv6Address
  Transition6Rd::Get6rdNetwork (Ipv4Address nodeAddress, Ipv6Address subnetId,
				uint32_t subLength)
  {

    uint8_t ispPrefix = m_ispNetwork.GetPrefix ().GetPrefixLength ();
    NS_ASSERT_MSG((subLength + (32 - m_ipv4Prefix) + ispPrefix) <= 64,
		  "network will be tool long: subnet > 64");

    uint32_t index = PrefixToIndex (m_ispNetwork.GetPrefix ());
    uint32_t a = index / 8;
    uint32_t b = index % 8;

    uint8_t subnet[16];
    uint8_t nw[16];
    memset (subnet, 0x00, 16);
    memset (nw, 0x00, 16);

    m_ispNetwork.GetAddress ().GetBytes (subnet);
    NS_LOG_DEBUG("Index " << index);
    for (uint32_t j = 0; j < a; ++j)
      {
	nw[j] = subnet[j];
      }

    nw[a] = subnet[a] & getBit (b);

    uint32_t sufix = nodeAddress.Get ();
    sufix <<= m_ipv4Prefix;
    sufix >>= m_ipv4Prefix;
    uint8_t ip4Byte;
    uint32_t suf = m_ipv4Prefix / 8;
    for (uint32_t j = suf; j < 4; j++)
      {
	ip4Byte = ((sufix >> ((3 - j) * 8)) & 0xff);
	nw[a + j - suf + 1] |= ip4Byte << (8 - b);
	nw[a - suf + j] |= ip4Byte >> b;
      }

    uint32_t nextByte = 32 - m_ipv4Prefix + index;
    a = nextByte / 8;
    b = nextByte % 8;

    uint8_t id[16];
    memset (id, 0x00, 16);
    subnetId.GetBytes (id);
    uint32_t c = subLength / 8;
    for (uint32_t j = 0; j <= c; j++)
      {

	nw[a + j + 1] |= id[j] << (8 - b);
	nw[a + j] |= id[j] >> b;
      }

    return Ipv6Address (nw);
  }

  void
  Transition6Rd::SetCustomerEdge (Ipv4Address outgoingAddress, Ptr<Node> cE)
  {
    Ptr<Ipv6L3Protocol> l3 = cE->GetObject<Ipv6L3Protocol> ();
    l3->Add6RdNetwork (m_ipv4Prefix, m_ispNetwork, m_bRAddress);

    Ptr<Ipv4> ipv4 = cE->GetObject<Ipv4> ();
    uint32_t interface = ipv4->GetInterfaceForAddress (outgoingAddress);
    Ptr<Ipv6StaticRouting> pseudoroute;
    Ipv6StaticRoutingHelper routingHelper;
    Ptr<Ipv6> ipv6 = cE->GetObject<Ipv6> ();
    pseudoroute = routingHelper.GetStaticRouting (ipv6);
    pseudoroute->AddNetworkRouteTo (m_ispNetwork.GetAddress (),
				    m_ispNetwork.GetPrefix (), interface, 1);

    //get destination BR anycast without subnetID
    Ipv4Address bRAddress = Ipv4Address (m_bRAddress);
    Ipv6Address hop = Get6rdNetwork (bRAddress, Ipv6Address ("::"), 0);
    pseudoroute->SetDefaultRoute (hop, interface);

  }
  uint32_t
  Transition6Rd::GetIpv4Dest (Ipv6Prefix ispPrefix, Ipv6Address destination,
			      uint8_t v4Subnet, Ipv4Address v4Address)
  {

    uint32_t index = PrefixToIndex (ispPrefix);
    uint32_t a = index / 8;
    uint32_t b = index % 8;
    uint32_t c = v4Subnet / 8;
    uint8_t buff[16];
    uint8_t v4Buff[4];
    memset (v4Buff, 0x00, 4);
    memset (buff, 0x00, 16);
    destination.GetBytes (buff);
    for (uint32_t i = c; i < 4; i++)
      {
	v4Buff[i] |= buff[a - 1 + i - c] << (8 - b);
	v4Buff[i] |= buff[a + i - c] >> b;

      }

    uint32_t sufix = 0;
    sufix |= v4Buff[0];
    sufix <<= 8;
    sufix |= v4Buff[1];
    sufix <<= 8;
    sufix |= v4Buff[2];
    sufix <<= 8;
    sufix |= v4Buff[3];
    uint32_t destAddress = v4Address.Get ();
    destAddress >>= 32 - v4Subnet;
    destAddress <<= 32 - v4Subnet;
    destAddress |= sufix;
    return destAddress;
  }
  uint8_t
  Transition6Rd::getBit (uint32_t shift)
  {
    uint8_t ret = 0xff;
    for (uint32_t i = 0; i < (8 - shift); i++)
      ret <<= 1;
    return ret;

  }
  uint32_t
  Transition6Rd::PrefixToIndex (Ipv6Prefix prefix) const
  {

    uint8_t prefixBits[16];
    prefix.GetBytes (prefixBits);

    for (int32_t i = 15; i >= 0; --i)
      {
	for (uint32_t j = 0; j < 8; ++j)
	  {
	    if (prefixBits[i] & 1)
	      {
		uint32_t index = IPV6_BITS - (15 - i) * 8 - j;
		NS_ABORT_MSG_UNLESS(
		    index > 0 && index < IPV6_BITS,
		    "Transition6Rd::PrefixToIndex(): Illegal Prefix");
		return index;
	      }
	    prefixBits[i] >>= 1;
	  }
      }
    NS_ASSERT_MSG(false, "Transitio6Rd::PrefixToIndex(): Impossible");
    return 0;
  }

}
