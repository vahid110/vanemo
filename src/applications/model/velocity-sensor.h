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

#ifndef VELOCITY_SENSOR_H
#define VELOCITY_SENSOR_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/mobility-module.h"
/*
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/net-device-container.h"
#include "ns3/mac48-address.h"

#include <map>
*/

namespace ns3 {

/**
 * \ingroup udpecho
 * \brief A Udp Echo client
 *
 * Every packet sent should be returned by the server and received here.
 */
class VelocitySensor : public Application
{
public:
	enum VelocityState
	{
		VS_UNKNOWN = 0,
		VS_DESCELERATING,
		VS_STOPPED,
		VS_ACCELARATING,
		VS_ONMOVE,
	};
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  VelocitySensor ();
  void RegisterVelocityCB(Callback<void, VelocitySensor::VelocityState> cb);
  Vector GetCurVelocity();
  VelocityState GetCurState();
private:
  Vector GetVelocity();
  void StoreVelocity(const Vector&);
  void StoreVelocity();
  void UpdateState();

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  struct State
  {
	  State()
		: m_v_state(VS_UNKNOWN)
		, m_velocity()
	  	, m_timestamp(0)
	{}
	  VelocityState m_v_state;
	  Vector m_velocity;
	  Time m_timestamp;
  };

  State m_cur_state;
  Callback<void, VelocitySensor::VelocityState> m_cb;
};

} // namespace ns3

#endif /* VELOCITY_SENSOR_H */
