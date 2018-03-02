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

#ifndef DUALSTACK_CONTAINER_H
#define DUALSTACK_CONTAINER_H

#include <stdint.h>

#include <vector>

#include "ns3/ipv6.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/deprecated.h"

namespace ns3
{

  /**
   * popis chyyba
   */
  class DualStackContainer
  {
  public:
    /**
     * \brief Container Const Iterator for pairs of Ipv6 smart pointer / Interface Index.
     */
    typedef std::vector<std::pair<Ptr<Ipv6>, uint32_t> >::const_iterator Iterator;

    /**
     * \brief Constructor.
     */
    DualStackContainer ();

    uint32_t
    GetNV6 (void) const;
    Iterator
    BeginV6 (void) const;
    Iterator
    EndV6 (void) const;

    uint32_t
    GetNV4 (void) const;
    Iterator
    BeginV4 (void) const;
    Iterator
    EndV4 (void) const;
    /**
     * \brief Get the ipv4  interface index the specified node index.
     * \param i index of the node
     * \return interface index
     */
    uint32_t
    GetInterfaceIndexV4 (uint32_t i) const;

    /**
     * \brief Get the ipv6  interface index the specified node index.
     * \param i index of the node
     * \return interface index
     */
    uint32_t
    GetInterfaceIndexV6 (uint32_t i) const;
    /**
     * \brief Get the address for the specified index.
     * \param i interface index
     * \param j address index, generally index 0 is the link-local address
     * \return IPv6 address
     */
    Ipv6Address
    GetIpv6Address (uint32_t i, uint32_t j) const;

    /**
      * \brief Get the address for the specified index.
      * \param i interface index
      * \param j address index, generally index 0 is the link-local address
      * \return IPv4 address
      */
    Ipv4Address
    GetIpv4Address (uint32_t i, uint32_t j = 0) const;

    /**
     * \brief Fusion with another Container.
     * \param c container
     */
    void
    Add (DualStackContainer& c);

    /**
     * \brief Add a couple of name/interface.
     * \param  name of a node
     * \param interface interface index to add
     */
    void
    Add (std::string ipv6Name, uint32_t interface);

    /**
     * \brief Add a couple of name/interface.
     * \param  name of a node
     * \param interface interface index to add
     */
    void
    Add (Ptr<Ipv4> ipv4, uint32_t interface);
    /**
     * \brief Set the state of the stack (act as a router or as an host) for the specified index.
     * This automatically sets all the node's interfaces to the same forwarding state.
     * \param i index
     * \param state true : is a router, false : is an host
     */
    /**
     * \brief Add a couple IPv6/interface.
     * \param ipv6 IPv6 address
     * \param interface interface index
     */
    void
    Add (Ptr<Ipv6> ipv6, uint32_t interface);

    /**
     * \brief set forwarding for router
     * \param i router index
     * \param state true if forwarding
     */
    void
    SetForwarding (uint32_t i, bool state);

    /**
     * \brief set forwarding and 6to4 router state
     * \param i router index
     * \param state true if forwarding
     */
    void
    Set6to4Forwarding (uint32_t i, bool state);

    /**
     * \brief Set the state of the stack (act as a 6to4 router) for the specified index.
     * This automatically sets all the node's interfaces to the same forwarding state.
     * \param i index
     * \param state true : is a 6to4 router, false normal router
     */
    void
    Set6to4Router (uint32_t i, bool Router6to4);

    /**
     * \brief set 6to4 relay
     * \param router router to be set
     */
    void
    Set6to4Relay (uint32_t router);

    /**
     * \brief set default route to router in entire container for IPv6 routing
     * \param router container router
     */
    void
    SetDefaultRouteInAllNodes (uint32_t router);

    /**
     * \brief Set the default route for the specified index.
     * \param i index
     * \param router the default router
     */
    void
    SetDefaultRoute (uint32_t i, uint32_t router);

    /**
     * \brief Set the default route for the specified index.
     * Note that the route will be set to the link-local address of the node with the specified address.
     * \param i index
     * \param routerAddr the default router address
     */
    void
    SetDefaultRoute (uint32_t i, Ipv6Address routerAddr);

    /**
     * \brief get link local address
     * \param index index of address in container
     * \returns link local address
     */
    Ipv6Address
    GetLinkLocalAddress (uint32_t index);

  private:
    /**
     * \internal
     * \brief Container for pairs of Ipv6 smart pointer / Interface Index.
     */
    typedef std::vector<std::pair<Ptr<Ipv6>, uint32_t> > InterfaceVectorV6;

    /**
     * \internal
     * \brief Container for pairs of Ipv6 smart pointer / Interface Index.
     */
    typedef std::vector<std::pair<Ptr<Ipv4>, uint32_t> > InterfaceVectorV4;

    /**
     * \internal
     * \brief List of stacks and interfaces index.
     */
    InterfaceVectorV6 m_interfaces;
    InterfaceVectorV4 m_ipv4interfaces;

  };

} /* namespace ns3  */

#endif /* DUALSTACK_CONTAINER_H */
