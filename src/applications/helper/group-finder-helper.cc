/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "udp-echo-helper.h"
#include "ns3/group-finder.h"
#include "ns3/group-finder-helper.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/log.h"

namespace ns3 {


void
GroupFinderHelper::SetEnable(bool value)
{
    GroupFinder::SetEnable(value);
}

bool
GroupFinderHelper::IsEnabled()
{
    return GroupFinder::IsEnabled();
}

GroupFinderHelper::GroupFinderHelper (bool VSRequired)
	: m_prerequire_velocity_sensor(VSRequired)
{
  m_factory.SetTypeId (GroupFinder::GetTypeId ());
}

void 
GroupFinderHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
GroupFinderHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
GroupFinderHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
GroupFinderHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

void
GroupFinderHelper::SetGroup(NetDeviceContainer c)
{
    m_devices = c;
}

Ptr<Application>
GroupFinderHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<GroupFinder> app = m_factory.Create<GroupFinder> ();
  app->SetGroup(m_devices);
  node->AddApplication (app);
  if( m_prerequire_velocity_sensor)
  {
	  Ptr<VelocitySensor> vs;
      for (uint32_t i = 0; i < node->GetNApplications (); i++)
      {
    	  Ptr<Application> app = node->GetApplication (i);
    	  vs = DynamicCast<VelocitySensor> (app);
    	  if (vs)
    		  break;
      }

	  NS_ASSERT_MSG(vs, "Velocity Sensor Application should be installed First. Node:" << node->GetId() );
	  VelocitySensor::state_change_notifier_t cb = MakeCallback(&GroupFinder::MobilityStateUpdated, app);
	  vs->RegisterVelocityCB(cb);

  }
  GroupFinder::AddMacNodeMap(Mac48Address::ConvertFrom(
                                          node->GetDevice(1)->GetAddress()),
                                                          node);
  return app;
}

} // namespace ns3
