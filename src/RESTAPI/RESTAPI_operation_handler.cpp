//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_operation_handler.h"
#include "SimStats.h"
#include "SimulationCoordinator.h"
#include "Daemon.h"

namespace OpenWifi {
	void RESTAPI_operation_handler::DoPost() {

        auto Id = GetBinding("id","");
        if(Id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }

		std::string Op;
		if (!HasParameter("operation", Op) || (Op != "start" && Op != "stop" && Op != "cancel")) {
			return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
		}

		std::string SimId;
		if (!HasParameter("runningId", SimId) && Op!="start") {
			return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
		}

		auto Error=OpenWifi::RESTAPI::Errors::SUCCESS;
		if (Op == "start") {
			if(Daemon()->Master()) {
				if (SimulationCoordinator()->IsSimulationRunning(Id)) {
					return BadRequest(RESTAPI::Errors::SimulationIsAlreadyRunning);
				}
				SimulationCoordinator()->StartSim(SimId, Id, Error, UserInfo_.userinfo);
			} else {
				auto joinRunningId = GetParameter("joinRunningId");
				auto masterURI = GetParameter("masterURI");
				auto accessKey = GetParameter("accessKey");
				auto index = GetParameter("index",0);
				Logger_.information(fmt::format("Starting simulation {} from master {} index:{}, Key:{}, Offset:{}, Size:{}",
												 joinRunningId, masterURI, index, accessKey, QB_.Offset, QB_.Limit));
				if(QB_.Offset==0 || QB_.Limit==0 || joinRunningId.empty() || masterURI.empty() || accessKey.empty() || index==0) {
					return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
				}
				std::cout << "Starting simulation " << SimId << " from master " << masterURI << " index:" << index << std::endl;
				SimulationCoordinator()->StartSim(joinRunningId, Id, Error, UserInfo_.userinfo, masterURI, accessKey, QB_.Offset, QB_.Limit, index);
				SimId = joinRunningId;
			}
		} else if (Op == "stop") {
			SimulationCoordinator()->StopSim(SimId, Error, UserInfo_.userinfo);
		} else if (Op == "cancel") {
			SimulationCoordinator()->CancelSim(SimId, Error, UserInfo_.userinfo);
		}

		if (Error.err_num==OpenWifi::RESTAPI::Errors::SUCCESS.err_num) {
			OWLSObjects::SimulationStatus S;
			SimStats()->GetCurrent(SimId,S, UserInfo_.userinfo);
			Poco::JSON::Object Answer;
			S.to_json(Answer);
			return ReturnObject(Answer);
		}
		return BadRequest(Error);
	}
} // namespace OpenWifi