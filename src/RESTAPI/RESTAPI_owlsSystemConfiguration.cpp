//
// Created by stephane bourque on 2023-11-10.
//

#include "RESTAPI_owlsSystemConfiguration.h"
#include "Daemon.h"
#include "framework/MicroServiceFuncs.h"
#include "SimStats.h"

namespace OpenWifi {

	void RESTAPI_owlsSystemConfiguration::DoGet() {
		Poco::JSON::Object Answer;

		Answer.set("master", Daemon()->Master());
		Answer.set("publicURI", MicroServicePublicEndPoint());
		Answer.set("privateURI", MicroServicePrivateEndPoint());
		Answer.set("version", MicroServiceVersion());
		std::vector<OWLSObjects::SimulationStatus>  Statuses;
		SimStats()->GetAllSimulations(Statuses, UserInfo_.userinfo);
		Poco::JSON::Array   Arr;
		for(const auto &status:Statuses) {
			Poco::JSON::Object  Obj;
			status.to_json(Obj);
			Arr.add(Obj);
		}
		Answer.set("simulations", Arr);
		return ReturnObject(Answer);
	}

} // namespace OpenWifi