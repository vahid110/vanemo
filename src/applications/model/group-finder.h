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
#include <set>

namespace ns3 {

class Socket;
class Packet;
class WifiMacHeader;

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
  static void AddPmipMac(const Mac48Address&, Ptr<Node> );
  static Ptr<Node> GetNodeByPmipMac(const Mac48Address&);
  static Ptr<GroupFinder> GetGroupFinderApplication(Ptr<Node>);

  void SetGroup(NetDeviceContainer);
  const std::set<Mac48Address>& GetGroup() const;
  void SetBindMag(const Ipv6Address&);
  Ipv6Address GetBindMag() const;
  void SetGrpLeader(bool);
  bool GetGrpLeader() const;
  void MobilityStateUpdated(const VelocitySensor::MobilityState,
		  	  	  	  	    const VelocitySensor::MobilityState);
  void GroupBCastReceived(Ptr<Packet> packet, WifiMacHeader const *hdr);

  virtual ~GroupFinder ();

protected:
  virtual void DoDispose (void);

private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop
  void Report();
private:
  //Accompanying devices (excluding the node itself.
  NetDeviceContainer m_devices;
  std::set<Mac48Address> m_curMeshMacs;//internal mesh address
  std::set<Mac48Address> m_curPmipMacs;//internal pmip address
  Ipv6Address m_bind_mag;
  bool m_is_grp_leader;
  VelocitySensor::MobilityState m_curMobilityState;
  uint32_t m_reportInterval;
  EventId m_updateEvent;

  static bool m_enable;
  static std::map<Mac48Address, Ptr<Node> > m_meshMacToNode;
  static std::map<Mac48Address, Ptr<Node> > m_pmipMacToNode;
  static std::map<Ptr<Node> , Mac48Address> m_pmipNodeToMac;
};

} // namespace ns3

#endif /* GROUP_FINDER_CLIENT_H */
