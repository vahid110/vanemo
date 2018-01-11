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

#include "ns3/core-module.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/mobility-module.h"

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
  void RegisterVelocityCB(Callback<void,
		                           VelocitySensor::VelocityState,
								   VelocitySensor::VelocityState> cb);
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
  struct State
  {
	  State()
		: m_v_state(VS_UNKNOWN)
	  	, m_factor(0)
		, m_velocity()
	  {
		  TimeIt();
	  }

	  State(const Vector& v)
		: m_v_state(VS_UNKNOWN)
		, m_velocity(v)
	  {
	      m_factor = v.x * v.x + v.y * v.y + v.z * v.z;
	      TimeIt();
	  }

	  VelocityState m_v_state;
	  double m_factor;
	  Vector m_velocity;
	  Time m_timestamp;
  private:
	  void TimeIt()
	  {
		  m_timestamp =  Simulator::Now ();
	  }
  };

  State m_cur_state;
  Callback<void,
           VelocitySensor::VelocityState,
		   VelocitySensor::VelocityState> m_state_change_notifier;
};

} // namespace ns3

#endif /* VELOCITY_SENSOR_H */
