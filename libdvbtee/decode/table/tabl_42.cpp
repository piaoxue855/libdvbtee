/*****************************************************************************
 * Copyright (C) 2011-2015 Michael Ira Krufky
 *
 * Author: Michael Ira Krufky <mkrufky@linuxtv.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  42110-1301  USA
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include "decoder.h"

#include "functions.h"

#include "tabl_42.h"

#define TABLEID 0x42

#define CLASS_MODULE "[SDT]"

//#define dprintf(fmt, arg...) __dprintf(DBG_DECODE, fmt, ##arg)
#define dprintf(fmt, arg...) fprintf(stderr, fmt"\n", ##arg)

using namespace dvbtee::decode;
using namespace dvbtee::value;

static std::string TABLE_NAME = "SDT";

static std::string SDTSVC = "SDTSVC";

void sdt::store(dvbpsi_sdt_t *p_sdt)
#define SDT_DBG 1
{
#if USING_DVBPSI_VERSION_0
	uint16_t __ts_id = p_sdt->i_ts_id;
#else
	uint16_t __ts_id = p_sdt->i_extension;
#endif
//	if ((decoded_sdt->version    == p_sdt->i_version) &&
//	    (decoded_sdt->network_id == p_sdt->i_network_id)) {

//		dprintf("SDT v%d | ts_id %d | network_id %d: ALREADY DECODED",
//			p_sdt->i_version,
//			__ts_id,
//			p_sdt->i_network_id);
//		return false;
//	}
	fprintf(stderr, "%s SDT: v%02d, ts_id %05d, network_id %05d\n"
		/*"------------------------------------"*/, __func__,
		p_sdt->i_version,
		__ts_id,
		p_sdt->i_network_id);

	decoded_sdt.ts_id      = __ts_id;
	decoded_sdt.version    = p_sdt->i_version;
	decoded_sdt.network_id = p_sdt->i_network_id;

	set("tsId", __ts_id);
	set("version", p_sdt->i_version);
	set("networkId", p_sdt->i_network_id);

	Array services;

	services_w_eit_pf    = 0;
	services_w_eit_sched = 0;

	//fprintf(stderr, "  service_id | service_name");
	dvbpsi_sdt_service_t* p_service = p_sdt->p_first_service;
	if (p_service)
		dprintf(" svcId | EIT avail |  provider  | service name");
	while (p_service) {

		decoded_sdt_service_t &cur_service = decoded_sdt.services[p_service->i_service_id];

		sdtSVC *sdtSvc = new sdtSVC(cur_service, this, p_service);
		if (sdtSvc->isValid())
			services.push((Object*)sdtSvc);

		if (cur_service.f_eit_present)  services_w_eit_pf++;
		if (cur_service.f_eit_sched) services_w_eit_sched++;

		p_service = p_service->p_next;
	}

	set("services", services);

	setValid(true);

#if 0
	dprintf("%s", toJson().c_str());
#endif

	if ((/*changed*/true) && (m_watcher)) {
		m_watcher->updateTable(TABLEID, (Table*)this);
	}
}

sdtSVC::sdtSVC(decoded_sdt_service_t& cur_service, Decoder *parent, dvbpsi_sdt_service_t *p_service)
: TableDataComponent(parent, SDTSVC)
{
	if (!p_service) return;

	cur_service.service_id     = p_service->i_service_id; /* matches program_id / service_id from PAT */
	cur_service.f_eit_sched    = p_service->b_eit_schedule;
	cur_service.f_eit_present  = p_service->b_eit_present;
	cur_service.running_status = p_service->i_running_status;
	cur_service.f_free_ca      = p_service->b_free_ca;

	set("serviceId",     p_service->i_service_id);
	set("f_eit_sched",   p_service->b_eit_schedule);
	set("f_eit_present", p_service->b_eit_present);
	set("runningStatus", p_service->i_running_status);
	set("f_free_ca",     p_service->b_free_ca);

	/* service descriptors contain service provider name & service name */
	descriptors.decode(p_service->p_first_descriptor);
	if (descriptors.size()) set<Array>("descriptors", descriptors);

	memset((void*)cur_service.provider_name, 0, sizeof(cur_service.provider_name));
	memset((void*)cur_service.service_name, 0, sizeof(cur_service.service_name));

	const dvbtee::decode::Descriptor *d = descriptors.last(0x48);
	if (d) {
		strncpy((char*)cur_service.provider_name,
			d->get<std::string>("providerName").c_str(),
			sizeof(cur_service.provider_name));
		strncpy((char*)cur_service.service_name,
			d->get<std::string>("serviceName").c_str(),
			sizeof(cur_service.service_name));
	}

	dprintf(" %05d | %s %s | %s - %s",
		cur_service.service_id,
		(cur_service.f_eit_present) ? "p/f" : "   ",
		(cur_service.f_eit_sched) ? "sched" : "     ",
		cur_service.provider_name,
		cur_service.service_name);

	setValid(true);
}

sdtSVC::~sdtSVC()
{

}


bool sdt::ingest(TableStore *s, dvbpsi_sdt_t *t, TableWatcher *w)
{
	const std::vector<Table*> sdts = s->get(TABLEID);
	for (std::vector<Table*>::const_iterator it = sdts.begin(); it != sdts.end(); ++it) {
		sdt *thisPmt = (sdt*)*it;
		if (thisPmt->get<uint16_t>("networkId") == t->i_network_id) {
			if (thisPmt->get<uint16_t>("version") == t->i_version) {
				dprintf("SDT v%d, network_id %d: ALREADY DECODED", t->i_version, t->i_network_id);
				return false;
			}
			thisPmt->store(t);
			return true;
		}
	}
	return s->add<dvbpsi_sdt_t>(TABLEID, t, w);
}


sdt::sdt(Decoder *parent, TableWatcher *watcher)
 : Table(parent, TABLE_NAME, TABLEID, watcher)
 , services_w_eit_pf(0)
 , services_w_eit_sched(0)
{
	//store table later (probably repeatedly)
}

sdt::sdt(Decoder *parent, TableWatcher *watcher, dvbpsi_sdt_t *p_sdt)
 : Table(parent, TABLE_NAME, TABLEID, watcher)
 , services_w_eit_pf(0)
 , services_w_eit_sched(0)
{
	store(p_sdt);
}

sdt::~sdt()
{
	//
}

REGISTER_TABLE_FACTORY(TABLEID, dvbpsi_sdt_t, sdt);