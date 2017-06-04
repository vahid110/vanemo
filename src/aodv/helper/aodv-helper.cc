/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 * Authors: Pavel Boyko <boyko@iitp.ru>, written after OlsrHelper by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "aodv-helper.h"
#include "ns3/aodv-routing-protocol.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv6-list-routing.h"

namespace ns3
{

AodvHelper::AodvHelper() : 
  Ipv4RoutingHelper ()
{
  m_agentFactory.SetTypeId ("ns3::aodv::RoutingProtocol");
}

AodvHelper* 
AodvHelper::Copy (void) const 
{
  return new AodvHelper (*this); 
}

Ptr<Ipv4RoutingProtocol> 
AodvHelper::Create (Ptr<Node> node) const
{
  Ptr<aodv::RoutingProtocol> agent = m_agentFactory.Create<aodv::RoutingProtocol> ();
  node->AggregateObject (agent);
  return agent;
}

void 
AodvHelper::Set (std::string name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}

int64_t
AodvHelper::AssignStreams (NodeContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<Node> node;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      node = (*i);
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      NS_ASSERT_MSG (ipv4, "Ipv4 not installed on node");
      Ptr<Ipv4RoutingProtocol> proto = ipv4->GetRoutingProtocol ();
      NS_ASSERT_MSG (proto, "Ipv4 routing not installed on node");
      Ptr<aodv::RoutingProtocol> aodv = DynamicCast<aodv::RoutingProtocol> (proto);
      if (aodv)
        {
          currentStream += aodv->AssignStreams (currentStream);
          continue;
        }
      // Aodv may also be in a list
      Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting> (proto);
      if (list)
        {
          int16_t priority;
          Ptr<Ipv4RoutingProtocol> listProto;
          Ptr<aodv::RoutingProtocol> listAodv;
          for (uint32_t i = 0; i < list->GetNRoutingProtocols (); i++)
            {
              listProto = list->GetRoutingProtocol (i, priority);
              listAodv = DynamicCast<aodv::RoutingProtocol> (listProto);
              if (listAodv)
                {
                  currentStream += listAodv->AssignStreams (currentStream);
                  break;
                }
            }
        }
    }
  return (currentStream - stream);
}

Aodv6Helper::Aodv6Helper() :
  Ipv6RoutingHelper ()
{
  m_agentFactory.SetTypeId ("ns3::aodv::RoutingProtocol6");
}

Aodv6Helper*
Aodv6Helper::Copy (void) const
{
  return new Aodv6Helper (*this);
}

Ptr<Ipv6RoutingProtocol>
Aodv6Helper::Create (Ptr<Node> node) const
{
  Ptr<aodv::RoutingProtocol6> agent = m_agentFactory.Create<aodv::RoutingProtocol6> ();
  node->AggregateObject (agent);
  return agent;
}

void
Aodv6Helper::Set (std::string name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}

int64_t
Aodv6Helper::AssignStreams (NodeContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<Node> node;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      node = (*i);
      Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
      NS_ASSERT_MSG (ipv6, "Ipv6 not installed on node");
      Ptr<Ipv6RoutingProtocol> proto = ipv6->GetRoutingProtocol ();
      NS_ASSERT_MSG (proto, "Ipv6 routing not installed on node");
      Ptr<aodv::RoutingProtocol6> aodv = DynamicCast<aodv::RoutingProtocol6> (proto);
      if (aodv)
        {
          currentStream += aodv->AssignStreams (currentStream);
          continue;
        }
      // Aodv may also be in a list
      Ptr<Ipv6ListRouting> list = DynamicCast<Ipv6ListRouting> (proto);
      if (list)
        {
          int16_t priority;
          Ptr<Ipv6RoutingProtocol> listProto;
          Ptr<aodv::RoutingProtocol6> listAodv;
          for (uint32_t i = 0; i < list->GetNRoutingProtocols (); i++)
            {
              listProto = list->GetRoutingProtocol (i, priority);
              listAodv = DynamicCast<aodv::RoutingProtocol6> (listProto);
              if (listAodv)
                {
                  currentStream += listAodv->AssignStreams (currentStream);
                  break;
                }
            }
        }
    }
  return (currentStream - stream);
}

}
