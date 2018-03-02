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

#ifndef TRANSITION_6IN4_H
#define TRANSITION_6IN4_H

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

namespace ns3
{

  class Transition6In4
  {
  public:

    /**
     * \brief constructor
     */
    Transition6In4 ();

    /**
     * \brief set 6in4 tunnel
     * \param tunnelSource first end of tunnel
     * \param tunnelDestination second end of tunnel
     * \param sourceNetwork first end IPv6 network
     * \param destinationNetwork second end IPv6 network
     * \param sourceAddress address of first end
     * \param destAddress   address of second end
     */
    void
    Set6In4 (Ptr<Node> tunnelSource, Ptr<Node> tunnelDestination,
	     std::string sourceNetwork, std::string destinationNetwork,
	     Ipv4Address sourceAddress, Ipv4Address destAddress);
    /**
     * \brief get information if gateway go to 6in4 tunnel
     * \param gateway IPv6 address of gateway
     * \returns true if is gateway to tunnel
     */
    bool
    Is6In4Gateway (Ipv6Address gateway);
  };

}

#endif
