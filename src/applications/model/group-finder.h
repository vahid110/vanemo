/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */

#ifndef GROUP_FINDER_CLIENT_H
#define GROUP_FINDER_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/net-device-container.h"
#include "ns3/mac48-address.h"
#include "ns3/velocity-sensor.h"

#include <map>

namespace ns3 {

class Socket;
class Packet;

/**
 * Find Groups of nodes.
 */
class GroupFinder : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  GroupFinder ();
  static void SetEnable(bool);
  static bool IsEnabled();
  static void AddMacNodeMap(const Mac48Address&, Ptr<Node> );
  static Ptr<Node> GetNodebyMac(const Mac48Address&);
  static Ptr<GroupFinder> GetGroupFinderApplication(Ptr<Node>);

  void SetGroup(NetDeviceContainer);
  NetDeviceContainer GetGroup() const;
  void SetBindMag(const Ipv6Address&);
  Ipv6Address GetBindMag() const;
  void SetGrpLeader(bool);
  bool GetGrpLeader() const;

  virtual ~GroupFinder ();

protected:
  virtual void DoDispose (void);

private:
  void MobilityStateUpdated(const VelocitySensor::VelocityState,
		  	  	  	  	    const VelocitySensor::VelocityState);
  //Accompanying devices (excluding the node itself.
  NetDeviceContainer m_devices;
  Ipv6Address m_bind_mag;
  bool m_is_grp_leader;
  VelocitySensor::VelocityState m_cur_mobility;

  static bool m_enable;
  static std::map<Mac48Address, Ptr<Node> > m_mac_to_node;
};

} // namespace ns3

#endif /* GROUP_FINDER_CLIENT_H */
