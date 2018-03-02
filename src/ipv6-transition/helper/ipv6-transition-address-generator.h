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

#ifndef IPV6_TRANSITION_ADDRESS_GENERATOR_H
#define IPV6_TRANSITION_ADDRESS_GENERATOR_H

#include <vector>

#include "ns3/ipv6-address.h"
#include "ns3/net-device-container.h"
#include "ns3/dualstack-container.h"
#include "ns3/deprecated.h"

#include "ns3/ipv4.h"
#include "ns3/ipv4-address-generator.h"

namespace ns3
{

  class Ipv6TransitionAddressGenerator
  {
  public:
    /**
     * \brief Constructor.
     */
    Ipv6TransitionAddressGenerator ();

    Ipv6TransitionAddressGenerator (Ipv6Address network, Ipv6Prefix prefix,
				    Ipv6Address base);

    /**
     * @brief Set the base network number, network mask and base address.
     *
     * The address helper allocates IP addresses based on a given network number
     * and mask combination along with an initial IP address.
     *
     * For example, if you want to use a /24 prefix with an initial network number
     * of 192.168.1 (corresponding to a mask of 255.255.255.0) and you want to
     * start allocating IP addresses out of that network beginning at 192.168.1.3,
     * you would call
     *
     *   SetBase ("192.168.1.0", "255.255.255.0", "0.0.0.3");
     *
     * If you don't care about the initial address it defaults to "0.0.0.1" in
     * which case you can simply use,
     *
     *   SetBase ("192.168.1.0", "255.255.255.0");
     *
     * and the first address generated will be 192.168.1.1.
     *
     * \param network The Ipv4Address containing the initial network number to
     * use during allocation.  The bits outside the network mask are not used.
     * \param mask The Ipv4Mask containing one bits in each bit position of the
     * network number.
     * \returns Nothing.
     */
    void
    SetIpv4Base (Ipv4Address network, Ipv4Mask mask, Ipv4Address Base);

    /**
     * \brief set 6to4 address out of IPv4 address and init address if mac=true
     * \param ip IPv4 address of network router
     * \param init initialization address
     * \param mac false, then generate address from init address else autoconfiguration EUI
     */
    void
    Set6To4Addr (std::string ip, std::string init, bool mac);

    /**
     * \brief set only IPv6 base address for generation
     * \param network IPv6 network
     * \param init initialization address
     * \param mac false, then generate address from init address else autoconfiguration EUI
     */
    void
    SetIpv6Base (std::string network, std::string init, bool mac);

    /**
     * \brief set only IPv6 base address for generation
     * \param network IPv6 network
     * \param init initialization address
     */
    void
    SetIpv6Base (Ipv6Address network, Ipv6Address init);

    /**
     * \brief assign only IPv4 adressess to network
     * \param c container to be added to dual stack container
     * \param excludeAddress address of 6to4 router if needed
     */
    DualStackContainer
    AssignIpv4Only (const NetDeviceContainer &c, Ipv4Address excludeAddress);

    /**
     * \brief assign only IPv4 adressess to network
     * \param c container to be added to dual stack container
     * \returns interface container
     */
    DualStackContainer
    AssignIpv4Only (const NetDeviceContainer &c);

    /**
     * \brief assign only IPv4 addresses to network
     * \param c container to be added to dual stack container
     * \param excludeAddress address of 6to4 router if needed
     * \returns interface container
     */
    DualStackContainer
    Assign6to4Ipv4Only (const NetDeviceContainer &c, uint32_t router6to4,
			Ipv4Address addr6to4);
    /**
     * \brief get IPv4 network
     * \returns IPv4 network address
     */
    Ipv4Address
    GetIpv4Addr ();

    /**
     * \brief assign IPv6 addresses and IPv4 addresses
     * \param c container to be added to dual stack container
     * \returns interface container
     */
    DualStackContainer
    AssignDualStack (const NetDeviceContainer &c);

    /**
     * \brief assign only IPv6 addresses to network
     * \param c container to be added to dual stack container
     * \returns interface container
     */
    DualStackContainer
    AssignIpv6Only (const NetDeviceContainer &c);

  private:
    /**
     * \internal
     * \brief Container for pairs of Ipv6 smart pointer / Interface Index.
     */
    typedef std::vector<std::pair<Ptr<Ipv6>, uint32_t> > InterfaceVector;

    /**
     * Saved Ipv4 network
     */
    Ipv4Address m_ipv4addr;
    Ipv4Address m_ipv46to4addr;
    uint32_t m_v4network; //!< network address
    uint32_t m_v4mask;    //!< network mask
    uint32_t m_v4address; //!< address
    uint32_t m_v4base;    //!< base address
    uint32_t m_v4shift; //!< shift, equivalent to the number of bits in the hostpart
    uint32_t m_v4max;     //!< maximum allowed address
    uint8_t m_v6init[16];	//initial address of network
    bool m_mac;			// generating address from MAC

    /**
     * @internal
     * \brief get new IPv6 address
     * \param addr mac address of interface
     * \return new IPv6 address
     */
    Ipv6Address
    NewAddressIpv6 (Address addr);

    /**
     * @internal
     * \brief get new IPv4 address
     * \param addr mac address of interface
     * \return new IPv4 address
     */
    Ipv4Address
    NewAddressIpv4 (void);

    /**
     * @internal
     * \brief convert IP to int
     * \param ipAddress address to be converted
     * \return integer IP
     */
    uint32_t
    IpToInt (const std::string & ipAddress);

    /**
     * @internal
     * \brief increment initialization address while generating
     */
    void
    incInit ();

    /**
     * @internal
     * \brief Returns the number of address bits (hostpart) for a given netmask
     * \param maskbits the netmask
     * \returns the number of bits in the hostpart
     */
    uint32_t
    NumAddressBits (uint32_t maskbits) const;

    /**
     * @internal
     * \brief convert integer IP to hex
     * \param ipInteger integer IP
     * \returns hex in 8 bits pointer
     */
    char *
    ConvertIpIntegerToHex (unsigned long ipInteger);

  };

} /* namespace ns3  */

#endif /* ADDRESS_GENERATOR_FOR_6TO4_H */
