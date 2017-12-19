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

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup udpecho
 * \brief A Udp Echo client
 *
 * Every packet sent should be returned by the server and received here.
 */
class GroupFinder : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  static void SetEnable(bool);
  static bool IsEnabled();

  GroupFinder ();

  void SetGroup(NetDeviceContainer);
  NetDeviceContainer GetGroup() const;

  virtual ~GroupFinder ();

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);
  //Accompanying devices (excluding the node itself.
  NetDeviceContainer m_devices;
  static bool m_enable;
};

} // namespace ns3

#endif /* GROUP_FINDER_CLIENT_H */
