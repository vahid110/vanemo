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

#include "ipv6-transition-address-generator.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/mac16-address.h"
#include "ns3/mac48-address.h"
#include "ns3/mac64-address.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-address-generator.h"
#include "ns3/dualstack-container.h"

#include <cstdio>

#include "ns3/ipv4.h"
#include "ns3/ipv4-address-generator.h"
#include "ns3/simulator.h"

namespace ns3
{

  NS_LOG_COMPONENT_DEFINE("Ipv6TransitionAddressGenerator");

  Ipv6TransitionAddressGenerator::Ipv6TransitionAddressGenerator ()
  {
    NS_LOG_FUNCTION(this);
    Ipv6AddressGenerator::Init (Ipv6Address ("2002:6a4::"), Ipv6Prefix (64));

    //
    // Set the default values to an illegal state.  Do this so the client is
    // forced to think at least briefly about what addresses get used and what
    // is going on here.
    //
    m_v4network = 0xffffffff;
    m_v4mask = 0;
    m_v4address = 0xffffffff;
    m_v4base = 0xffffffff;
    m_v4shift = 0xffffffff;
    m_v4max = 0xffffffff;
    m_mac = true;
  }

  Ipv6TransitionAddressGenerator::Ipv6TransitionAddressGenerator (
      Ipv6Address network, Ipv6Prefix prefix, Ipv6Address base)
  {
    NS_LOG_FUNCTION(this << network << prefix << base);
    Ipv6AddressGenerator::Init (network, prefix, base);

    m_v4network = 0xffffffff;
    m_v4mask = 0;
    m_v4address = 0xffffffff;
    m_v4base = 0xffffffff;
    m_v4shift = 0xffffffff;
    m_v4max = 0xffffffff;
    m_mac = true;
  }

  void
  Ipv6TransitionAddressGenerator::SetIpv6Base (std::string network,
					       std::string init, bool mac)
  {
	    NS_LOG_FUNCTION(this);

    Ipv6Address in = Ipv6Address (init.c_str ());
    in.GetBytes (m_v6init);
    Ipv6Address net = Ipv6Address (network.c_str ());

    m_mac = mac;
    Ipv6AddressGenerator::Init (net, Ipv6Prefix (64), in);
  }
  void
  Ipv6TransitionAddressGenerator::SetIpv6Base (Ipv6Address network,
					       Ipv6Address init)
  {
	    NS_LOG_FUNCTION(this);

    init.GetBytes (m_v6init);
    m_mac = false;

    Ipv6AddressGenerator::Init (network, Ipv6Prefix (64), init);
  }

  void
  Ipv6TransitionAddressGenerator::Set6To4Addr (std::string ip, std::string init,
					       bool mac)
  {
	    NS_LOG_FUNCTION(this);

    Ipv6Address network;
    Ipv6Prefix prefix;

    m_ipv4addr = Ipv4Address (ip.c_str ());
    m_ipv46to4addr = Ipv4Address (ip.c_str ());
    m_mac = mac;
    char * hex = ConvertIpIntegerToHex (IpToInt (ip));
    char const* v4Addr = hex;

    network = Ipv6Address (v4Addr);
    //IPv6 implementation work bad with /48 prefix
    prefix = Ipv6Prefix (48);
    Ipv6Address in = Ipv6Address (init.c_str ());
    in.GetBytes (m_v6init);

    Ipv6AddressGenerator::Init (network, Ipv6Prefix (64), in);
  }

  Ipv4Address
  Ipv6TransitionAddressGenerator::NewAddressIpv4 (void)
  {
//
// The way this is expected to be used is that an address and network number
// are initialized, and then NewAddress() is called repeatedly to allocate and
// get new addresses on a given subnet.  The client will expect that the first
// address she gets back is the one she used to initialize the generator with.
// This implies that this operation is a post-increment.
//
    NS_ASSERT_MSG(
	m_v4address <= m_v4max,
	"Ipv6TransitionAddressGenerator::NewAddressIpv4: Address overflow");

    Ipv4Address addr ((m_v4network << m_v4shift) | m_v4address);
    ++m_v4address;
//
// The Ipv4AddressGenerator allows us to keep track of the addresses we have
// allocated and will assert if we accidentally generate a duplicate.  This
// avoids some really hard to debug problems.
//
    Ipv4AddressGenerator::AddAllocated (addr);
    NS_LOG_UNCOND("NewAddressIpv4: " << addr);
    return addr;
  }

  Ipv6Address
  Ipv6TransitionAddressGenerator::NewAddressIpv6 (Address addr)
  {
    NS_LOG_FUNCTION(this << addr);
    if (m_mac)
      {
	if (Mac64Address::IsMatchingType (addr))
	  {
	    Ipv6Address network = Ipv6AddressGenerator::GetNetwork (
		Ipv6Prefix (64));
	    Ipv6Address address = Ipv6Address::MakeAutoconfiguredAddress (
		Mac64Address::ConvertFrom (addr), network);

	    Ipv6AddressGenerator::AddAllocated (address);
	    return address;
	  }
	else if (Mac48Address::IsMatchingType (addr))
	  {
	    Ipv6Address network = Ipv6AddressGenerator::GetNetwork (
		Ipv6Prefix (64));
	    Ipv6Address address = Ipv6Address::MakeAutoconfiguredAddress (
		Mac48Address::ConvertFrom (addr), network);

	    Ipv6AddressGenerator::AddAllocated (address);
	    return address;
	  }
	else if (Mac16Address::IsMatchingType (addr))
	  {
	    Ipv6Address network = Ipv6AddressGenerator::GetNetwork (
		Ipv6Prefix (64));
	    Ipv6Address address = Ipv6Address::MakeAutoconfiguredAddress (
		Mac16Address::ConvertFrom (addr), network);
	    Ipv6AddressGenerator::AddAllocated (address);
	    return address;
	  }
	else
	  {
	    NS_FATAL_ERROR(
		"Did not pass in a valid Mac Address (16, 48 or 64 bits)");
	  }
      }
    else
      {
	Ipv6Address network = Ipv6AddressGenerator::GetNetwork (
	    Ipv6Prefix (64));
	Ipv6Address init;
	init.Set (m_v6init);
	Ipv6Address address = Ipv6Address::Make6To4Address (network, init);

	incInit ();
	Ipv6AddressGenerator::AddAllocated (address);
	return address;
      }
    /* never reached */
    return Ipv6Address ("::");
  }

  Ipv4Address
  Ipv6TransitionAddressGenerator::GetIpv4Addr ()
  {
	    NS_LOG_FUNCTION(this);
    Ipv4Address addr = m_ipv4addr;
    return addr;
  }

  void
  Ipv6TransitionAddressGenerator::SetIpv4Base (const Ipv4Address network,
					       const Ipv4Mask mask,
					       const Ipv4Address base)
  {
    NS_LOG_FUNCTION_NOARGS ();

    m_v4network = network.Get ();
    m_v4mask = mask.Get ();
    m_v4base = m_v4address = base.Get ();
//
// Some quick reasonableness testing.
//
    NS_ASSERT_MSG(
	(m_v4network & ~m_v4mask) == 0,
	"Ipv6TransitionAddressGenerator::SetIpv4Base: Inconsistent network and mask");

//
// Figure out how much to shift network numbers to get them aligned, and what
// the maximum allowed address is with respect to the current mask.
//
    m_v4shift = NumAddressBits (m_v4mask);
    m_v4max = (1 << m_v4shift) - 2;

    NS_ASSERT_MSG(
	m_v4shift <= 32,
	"Ipv6TransitionAddressGenerator::SetIpv4Base:: Unreasonable address length");

//
// Shift the network down into the normalized position.
//
    m_v4network >>= m_v4shift;

    NS_LOG_LOGIC("m_network == " << m_v4network);
    NS_LOG_LOGIC("m_mask == " << m_v4mask);
    NS_LOG_LOGIC("m_address == " << m_v4address);
  }

  DualStackContainer
  Ipv6TransitionAddressGenerator::Assign6to4Ipv4Only (
      const NetDeviceContainer &c,
	  uint32_t router6to4,
      const Ipv4Address addr6to4)
  {
    NS_LOG_FUNCTION_NOARGS ();
    DualStackContainer retval;
    for (uint32_t i = 0; i < c.GetN (); ++i)
      {

	Ptr<NetDevice> device = c.Get (i);

	Ptr<Node> node = device->GetNode ();
	NS_ASSERT_MSG(
	    node,
	    "Ipv6TransitionAddressGenerator::AssignIpv4Only(): NetDevice is not not associated " "with any node -> fail");
	Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
	NS_ASSERT_MSG(
	    ipv4,
	    "Ipv6TransitionAddressGenerator::AssignIpv4Only(): NetDevice is associated" " with a node without IPv4 stack installed -> fail " "(maybe need to use InternetStackHelper?)");

	int32_t interface = ipv4->GetInterfaceForDevice (device);
	if (interface == -1)
	  {
	    interface = ipv4->AddInterface (device);
	  }
	NS_ASSERT_MSG(
	    interface >= 0,
	    "Ipv6TransitionAddressGenerator::AssignIpv4Only(): " "Interface index not found");

	Ipv4InterfaceAddress ipv4Addr;

	if (i != router6to4)
	  {
	    Ipv4Address newAddr = NewAddressIpv4 ();
	    if (newAddr.IsEqual (addr6to4))
	      newAddr = NewAddressIpv4 ();
	    ipv4Addr = Ipv4InterfaceAddress (newAddr, m_v4mask);
	  }
	else
	  {
	    ipv4Addr = Ipv4InterfaceAddress (addr6to4, m_v4mask);
	  }
	ipv4->AddAddress (interface, ipv4Addr);
	ipv4->SetMetric (interface, 1);
	ipv4->SetUp (interface);

	retval.Add (ipv4, interface);
      }
    return retval;
  }

  DualStackContainer
  Ipv6TransitionAddressGenerator::AssignIpv4Only (const NetDeviceContainer &c,
						  Ipv4Address excludeAddress)
  {
    NS_LOG_FUNCTION_NOARGS ();
    DualStackContainer retval;
    for (uint32_t i = 0; i < c.GetN (); ++i)
      {

	Ptr<NetDevice> device = c.Get (i);

	Ptr<Node> node = device->GetNode ();
	NS_ASSERT_MSG(
	    node,
	    "Ipv6TransitionAddressGenerator::AssignIpv4Only(): NetDevice is not not associated " "with any node -> fail");
	Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

	NS_ASSERT_MSG(
	    ipv4,
	    "Ipv6TransitionAddressGenerator::AssignIpv4Only(): NetDevice is associated" " with a node without IPv4 stack installed -> fail " "(maybe need to use InternetStackHelper?)");

	int32_t interface = ipv4->GetInterfaceForDevice (device);
	if (interface == -1)
	  {
	    interface = ipv4->AddInterface (device);
	  }
	NS_ASSERT_MSG(
	    interface >= 0,
	    "Ipv6TransitionAddressGenerator::AssignIpv4Only(): " "Interface index not found");

	Ipv4InterfaceAddress ipv4Addr;
	Ipv4Address newAddr = NewAddressIpv4 ();

	if (newAddr.IsEqual (excludeAddress))
	  newAddr = NewAddressIpv4 ();

	ipv4Addr = Ipv4InterfaceAddress (newAddr, m_v4mask);

	ipv4->AddAddress (interface, ipv4Addr);
	ipv4->SetMetric (interface, 1);
	ipv4->SetUp (interface);

	retval.Add (ipv4, interface);
      }
    return retval;
  }
  DualStackContainer
  Ipv6TransitionAddressGenerator::AssignIpv4Only (const NetDeviceContainer &c)
  {
    NS_LOG_FUNCTION_NOARGS ();
    DualStackContainer retval;
    for (uint32_t i = 0; i < c.GetN (); ++i)
      {

	Ptr<NetDevice> device = c.Get (i);

	Ptr<Node> node = device->GetNode ();
	NS_ASSERT_MSG(
	    node,
	    "Ipv6TransitionAddressGenerator::AssignIpv4Only(): NetDevice is not not associated " "with any node -> fail");
	Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

	NS_ASSERT_MSG(
	    ipv4,
	    "Ipv6TransitionAddressGenerator::AssignIpv4Only(): NetDevice is associated" " with a node without IPv4 stack installed -> fail " "(maybe need to use InternetStackHelper?)");

	int32_t interface = ipv4->GetInterfaceForDevice (device);
	if (interface == -1)
	  {
	    interface = ipv4->AddInterface (device);
	  }
	NS_ASSERT_MSG(
	    interface >= 0,
	    "Ipv6TransitionAddressGenerator::AssignIpv4Only(): " "Interface index not found");

	Ipv4InterfaceAddress ipv4Addr;
	Ipv4Address newAddr = NewAddressIpv4 ();
	ipv4Addr = Ipv4InterfaceAddress (newAddr, m_v4mask);

	ipv4->AddAddress (interface, ipv4Addr);
	ipv4->SetMetric (interface, 1);
	ipv4->SetUp (interface);

	retval.Add (ipv4, interface);
      }
    return retval;
  }

  DualStackContainer
  Ipv6TransitionAddressGenerator::AssignDualStack (const NetDeviceContainer &c)
  {
	    NS_LOG_FUNCTION(this);

    DualStackContainer retval;

    for (uint32_t i = 0; i < c.GetN (); ++i)
      {
	Ptr<NetDevice> device = c.Get (i);

	Ptr<Node> node = device->GetNode ();
	NS_ASSERT_MSG(
	    node, "Ipv6TransitionAddressGenerator::AssignDualStack: Bad node");

	Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();

	Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
	NS_ASSERT_MSG(
	    ipv4,
	    "Ipv6TransitionAddressGenerator::AssignDualStack: NetDevice is associated" " with a node without IPv4 stack installed -> fail " "(maybe need to use InternetStackHelper?)");

	NS_ASSERT_MSG(
	    ipv6, "Ipv6TransitionAddressGenerator::AssignDualStack: Bad ipv6");
	int32_t ifIndex = 0;

	ifIndex = ipv6->GetInterfaceForDevice (device);
	int32_t interface = ipv4->GetInterfaceForDevice (device);
	if (ifIndex == -1)
	  {
	    ifIndex = ipv6->AddInterface (device);
	  }
	if (interface == -1)
	  {
	    interface = ipv4->AddInterface (device);
	  }
	NS_ASSERT_MSG(
	    ifIndex >= 0,
	    "Ipv6TransitionAddressGenerator::AssignDualStack: " "Interface index not found");
	NS_ASSERT_MSG(
	    interface >= 0,
	    "Ipv6TransitionAddressGenerator::AssignDualStack: " "Interface index not found");

	Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (
	    NewAddressIpv6 (device->GetAddress ()), Ipv6Prefix (64));

	Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (NewAddressIpv4 (),
							      m_v4mask);

	ipv6->SetMetric (ifIndex, 1);
	ipv6->AddAddress (ifIndex, ipv6Addr);
	ipv6->SetUp (ifIndex);
	retval.Add (ipv6, ifIndex);

	ipv4->AddAddress (interface, ipv4Addr);
	ipv4->SetMetric (interface, 1);
	ipv4->SetUp (interface);

	retval.Add (ipv4, interface);

      }
    return retval;
  }

  DualStackContainer
  Ipv6TransitionAddressGenerator::AssignIpv6Only (const NetDeviceContainer &c)
  {
	  NS_LOG_FUNCTION (this);
    DualStackContainer retval;

    for (uint32_t i = 0; i < c.GetN (); ++i)
      {
	Ptr<NetDevice> device = c.Get (i);

	Ptr<Node> node = device->GetNode ();
	NS_ASSERT_MSG(
	    node, "Ipv6TransitionAddressGenerator::AssignIpv6Only: Bad node");

	Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
	NS_ASSERT_MSG(
	    ipv6, "Ipv6TransitionAddressGenerator::AssignIpv6Only: Bad ipv6");
	int32_t ifIndex = 0;

	ifIndex = ipv6->GetInterfaceForDevice (device);
	if (ifIndex == -1)
	  {
	    ifIndex = ipv6->AddInterface (device);
	  }

	NS_ASSERT_MSG(
	    ifIndex >= 0,
	    "Ipv6TransitionAddressGenerator::AssignIpv6Only: " "Interface index not found");

	Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (
	    NewAddressIpv6 (device->GetAddress ()), Ipv6Prefix (64));
	ipv6->SetMetric (ifIndex, 1);
	ipv6->AddAddress (ifIndex, ipv6Addr);
	ipv6->SetUp (ifIndex);
	retval.Add (ipv6, ifIndex);

      }
    return retval;
  }

  uint32_t
  Ipv6TransitionAddressGenerator::IpToInt (const std::string & ipAddress)
  {
	NS_LOG_FUNCTION(this);
    const unsigned bitsPerTerm = 8;
    const unsigned numTerms = 4;

    std::istringstream ip (ipAddress);
    uint32_t packed = 0;

    for (unsigned i = 0; i < numTerms; ++i)
      {
	unsigned term;
	ip >> term;
	ip.ignore ();

	packed += term << (bitsPerTerm * (numTerms - i - 1));
      }

    return packed;
  }
  char *
  Ipv6TransitionAddressGenerator::ConvertIpIntegerToHex (
      unsigned long ipInteger)
  {
	NS_LOG_FUNCTION(this);
    static char HexIp[18];
    char c;
    int i, index = 5;

    strcpy (HexIp, "2002:");

    for (i = sizeof(ipInteger) - 1; i >= 0; i--)
      {
	if (i == 3)
	  {
	    HexIp[index++] = ':';
	  }
	c = "0123456789ABCDEF"[((ipInteger >> (i * 4)) & 0xF)];
	HexIp[index++] = c;
      }
    HexIp[index++] = ':';
    HexIp[index++] = ':';
    HexIp[index] = '\0'; //cap it

    return HexIp;
  }
  void
  Ipv6TransitionAddressGenerator::incInit ()
  {
    NS_LOG_FUNCTION(this);
    uint32_t byteprefix = 15;
    while (byteprefix != 8)
      {
	if (m_v6init[byteprefix] == 0xff)
	  m_v6init[byteprefix] = 0x00;
	else
	  {
	    m_v6init[byteprefix]++;
	    return;
	  }
	byteprefix--;
      }
    NS_ASSERT_MSG(
	false,
	"Ipv6TransitionAddressGenerator::incInit(): No more free addresses");
  }
  uint32_t
  Ipv6TransitionAddressGenerator::NumAddressBits (uint32_t maskbits) const
  {
    NS_LOG_FUNCTION_NOARGS ();
    for (uint32_t i = 0; i < 32; ++i)
      {
	if (maskbits & 1)
	  {
	    NS_LOG_LOGIC("NumAddressBits -> " << i);
	    return i;
	  }
	maskbits >>= 1;
      }

    NS_ASSERT_MSG(false,
		  "Ipv6TransitionAddressGenerator::NumAddressBits(): Bad Mask");
    return 0;
  }

}/* namespace ns3 */
