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
 * Based on
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      http://core.it.uu.se/core/index.php/AODV-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */

/****************************************Customized code****************************************/

#include "../model/aodv-neighbor.h"

#include "ns3/log.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AodvNeighbors");

namespace aodv {
Neighbors::Neighbors(Time delay) :
		m_ntimer(Timer::CANCEL_ON_DESTROY) {
	m_ntimer.SetDelay(delay);
	m_ntimer.SetFunction(&Neighbors::Purge, this);
	m_txErrorCallback = MakeCallback(&Neighbors::ProcessTxError, this);
}

Neighbors6::Neighbors6(Time delay) :
		m_ntimer(Timer::CANCEL_ON_DESTROY) {
	m_ntimer.SetDelay(delay);
	m_ntimer.SetFunction(&Neighbors6::Purge, this);
	m_WifitxErrorCallback = MakeCallback(&Neighbors6::ProcessWifiTxError, this);
	m_LrWpantxErrorCallback = MakeCallback(&Neighbors6::ProcessLrWpanTxError, this);
}

bool Neighbors::IsNeighbor(Ipv4Address addr) {
	Purge();
	for (std::vector<Neighbor>::const_iterator i = m_nb.begin();
			i != m_nb.end(); ++i) {
		if (i->m_neighborAddress == addr)
			return true;
	}
	return false;
}

bool Neighbors6::IsNeighbor(Ipv6Address addr) {
	Purge();
	for (std::vector<Neighbor>::const_iterator i = m_nb.begin();
			i != m_nb.end(); ++i) {
		if (i->m_neighborAddress == addr)
			return true;
	}
	return false;
}

Time Neighbors::GetExpireTime(Ipv4Address addr) {
	Purge();
	for (std::vector<Neighbor>::const_iterator i = m_nb.begin();
			i != m_nb.end(); ++i) {
		if (i->m_neighborAddress == addr)
			return (i->m_expireTime - Simulator::Now());
	}
	return Seconds(0);
}

Time Neighbors6::GetExpireTime(Ipv6Address addr) {
	Purge();
	for (std::vector<Neighbor>::const_iterator i = m_nb.begin();
			i != m_nb.end(); ++i) {
		if (i->m_neighborAddress == addr)
			return (i->m_expireTime - Simulator::Now());
	}
	return Seconds(0);
}

void Neighbors::Update(Ipv4Address addr, Time expire) {
	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i)
		if (i->m_neighborAddress == addr) {
			i->m_expireTime = std::max(expire + Simulator::Now(),
					i->m_expireTime);
			if (i->m_hardwareAddress == Mac48Address())
				i->m_hardwareAddress = LookupMacAddress(i->m_neighborAddress);
			return;
		}

	NS_LOG_LOGIC("Open link to " << addr);
	Neighbor neighbor(addr, LookupMacAddress(addr), expire + Simulator::Now());
	m_nb.push_back(neighbor);
	Purge();
}

void Neighbors6::Update(Ipv6Address addr, Time expire) {
	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end(); ++i)
		if (i->m_neighborAddress == addr) {
			i->m_expireTime = std::max(expire + Simulator::Now(),
					i->m_expireTime);
			if (i->m_hardwareAddress48 == Mac48Address())
			{
				NS_LOG_INFO("48 bit Mac address");
				i->m_hardwareAddress48 = LookupMac48Address(i->m_neighborAddress);
			}
			/*

			else if(i->m_hardwareAddress64 == Mac64Address())
			{
				NS_LOG_INFO("64 bit Mac address");
				i->m_hardwareAddress64 = LookupMac64Address(i->m_neighborAddress);
			}

			else if (i->m_hardwareAddress16 == Mac16Address())
			{
				NS_LOG_INFO("16 bit Mac address");
				i->m_hardwareAddress16 = LookupMac16Address(i->m_neighborAddress);
			}
			*/

			return;
		}

	NS_LOG_LOGIC("Open link to " << addr);
	if (LookupMac48Address(addr) == Mac48Address())
	{
		Neighbor neighbor(addr, LookupMac48Address(addr), expire + Simulator::Now());
		m_nb.push_back(neighbor);
	}
	/*
	else if (LookupMac64Address(addr) == Mac64Address())
	{
		Neighbor neighbor(addr, LookupMac64Address(addr), expire + Simulator::Now());
		m_nb.push_back(neighbor);
	}
	else if (LookupMac16Address(addr) == Mac16Address())
	{
		Neighbor neighbor(addr, LookupMac16Address(addr), expire + Simulator::Now());
		m_nb.push_back(neighbor);
	}
	else
	{
		NS_ASSERT_MSG(false, "Entry does not match with any Neighbor MAC format");
	}
	*/

	Purge();
}

struct CloseNeighbor {
	bool operator()(const Neighbors::Neighbor & nb) const {
		return ((nb.m_expireTime < Simulator::Now()) || nb.close);
	}
};

struct CloseNeighbor6 {
	bool operator()(const Neighbors6::Neighbor & nb) const {
		return ((nb.m_expireTime < Simulator::Now()) || nb.close);
	}
};

void Neighbors::Purge() {
	if (m_nb.empty())
		return;

	CloseNeighbor pred;
	if (!m_handleLinkFailure.IsNull()) {
		for (std::vector<Neighbor>::iterator j = m_nb.begin(); j != m_nb.end();
				++j) {
			if (pred(*j)) {
				NS_LOG_LOGIC("Close link to " << j->m_neighborAddress);
				m_handleLinkFailure(j->m_neighborAddress);
			}
		}
	}
	m_nb.erase(std::remove_if(m_nb.begin(), m_nb.end(), pred), m_nb.end());
	m_ntimer.Cancel();
	m_ntimer.Schedule();
}

void Neighbors6::Purge() {
	if (m_nb.empty())
		return;

	CloseNeighbor6 pred;
	if (!m_handleLinkFailure.IsNull()) {
		for (std::vector<Neighbor>::iterator j = m_nb.begin(); j != m_nb.end();
				++j) {
			if (pred(*j)) {
				NS_LOG_LOGIC("Close link to " << j->m_neighborAddress);
				m_handleLinkFailure(j->m_neighborAddress);
			}
		}
	}
	m_nb.erase(std::remove_if(m_nb.begin(), m_nb.end(), pred), m_nb.end());
	m_ntimer.Cancel();
	m_ntimer.Schedule();
}

void Neighbors::ScheduleTimer() {
	m_ntimer.Cancel();
	m_ntimer.Schedule();
}

void Neighbors6::ScheduleTimer() {
	m_ntimer.Cancel();
	m_ntimer.Schedule();
}

void Neighbors::AddArpCache(Ptr<ArpCache> a) {
	m_arp.push_back(a);
}

void Neighbors6::AddNdiscCache(Ptr<NdiscCache> a) {
	//NS_LOG_INFO("Adding entry of net device to ndiscCache");
	m_ndCache.push_back(a);
}

void Neighbors::DelArpCache(Ptr<ArpCache> a) {
	m_arp.erase(std::remove(m_arp.begin(), m_arp.end(), a), m_arp.end());
}

void Neighbors6::DelNdiscCache(Ptr<NdiscCache> a) {
	m_ndCache.erase(std::remove(m_ndCache.begin(), m_ndCache.end(), a),
			m_ndCache.end());
}

Mac48Address Neighbors::LookupMacAddress(Ipv4Address addr) {
	Mac48Address hwaddr;
	for (std::vector<Ptr<ArpCache> >::const_iterator i = m_arp.begin();
			i != m_arp.end(); ++i) {
		ArpCache::Entry * entry = (*i)->Lookup(addr);
		if (entry != 0 && entry->IsAlive() && !entry->IsExpired()) {
			hwaddr = Mac48Address::ConvertFrom(entry->GetMacAddress());
			break;
		}
	}
	return hwaddr;
}

Mac48Address Neighbors6::LookupMac48Address(Ipv6Address addr) {
	Mac48Address hwaddr;
	for (std::vector<Ptr<NdiscCache> >::const_iterator i = m_ndCache.begin();
			i != m_ndCache.end(); ++i) {
		NdiscCache::Entry * entry = (*i)->Lookup(addr);
		if (entry != 0 && entry->IsStale() && !entry->IsDelay()) {
			hwaddr = Mac48Address::ConvertFrom(entry->GetMacAddress());
			break;
		}
	}
	return hwaddr;
}
Mac64Address Neighbors6::LookupMac64Address(Ipv6Address addr) {
	Mac64Address hwaddr;
	for (std::vector<Ptr<NdiscCache> >::const_iterator i = m_ndCache.begin();
			i != m_ndCache.end(); ++i) {
		NdiscCache::Entry * entry = (*i)->Lookup(addr);
		if (entry != 0 && entry->IsStale() && !entry->IsDelay()) {
			hwaddr = Mac64Address::ConvertFrom(entry->GetMacAddress());
			break;
		}
	}
	return hwaddr;
}
Mac16Address Neighbors6::LookupMac16Address(Ipv6Address addr) {
	Mac16Address hwaddr;
	for (std::vector<Ptr<NdiscCache> >::const_iterator i = m_ndCache.begin();
			i != m_ndCache.end(); ++i) {
		NdiscCache::Entry * entry = (*i)->Lookup(addr);
		if (entry != 0 && entry->IsStale() && !entry->IsDelay()) {
			hwaddr = Mac16Address::ConvertFrom(entry->GetMacAddress());
			break;
		}
	}
	return hwaddr;
}

void Neighbors::ProcessTxError(WifiMacHeader const & hdr) {
	Mac48Address addr = hdr.GetAddr1();

	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end();
			++i) {
		if (i->m_hardwareAddress == addr)
			i->close = true;
	}
	Purge();
}

void Neighbors6::ProcessWifiTxError(WifiMacHeader const & hdr) {
	Mac48Address addr = hdr.GetAddr1();

	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end();
			++i) {
		if (i->m_hardwareAddress48 == addr)
			i->close = true;
	}
	Purge();
}

void Neighbors6::ProcessLrWpanTxError(LrWpanMacHeader const & hdr) {
	Mac64Address addrext = hdr.GetExtDstAddr();
	Mac16Address addr = hdr.GetShortDstAddr();

	for (std::vector<Neighbor>::iterator i = m_nb.begin(); i != m_nb.end();
			++i) {
		if (i->m_hardwareAddress16 == addr || i->m_hardwareAddress64 == addrext)
			i->close = true;
	}
	Purge();
}
}
}

