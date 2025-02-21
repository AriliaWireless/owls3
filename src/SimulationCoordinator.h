//
// Created by stephane bourque on 2021-11-03.
//

#pragma once

#include <chrono>
#include <random>

#include <framework/SubSystemServer.h>
#include <RESTObjects/RESTAPI_OWLSobjects.h>
#include <RESTObjects/RESTAPI_SecurityObjects.h>

#include "SimulationRunner.h"

namespace OpenWifi {

    struct SimulationRecord {
        SimulationRecord(const OWLSObjects::SimulationDetails & details,Poco::Logger &Logger, const std::string &RunningId,
						 const SecurityObjects::UserInfo &uinfo, const std::string &MasterURI,
						 const std::string &AccessKey, std::uint64_t Offset, std::uint64_t Limit, std::uint64_t Index):
                Details(details),
                Runner(details, Logger, RunningId, uinfo, MasterURI, AccessKey, Offset, Limit, Index),
                UInfo(uinfo){

        }
        std::atomic_bool                SimRunning = false;
        OWLSObjects::SimulationDetails  Details;
        SimulationRunner                Runner;
        SecurityObjects::UserInfo       UInfo;
		std::uint64_t 					Offset=0;
		std::uint64_t 					Limit=0;
    };

	class SimulationCoordinator : public SubSystemServer, Poco::Runnable {
	  public:
		static auto instance() {
			static auto instance_ = new SimulationCoordinator;
			return instance_;
		}

		int Start() final;
		void Stop() final;
		void run() final;

		bool StartSim(std::string &RunningId, const std::string &SimId, RESTAPI::Errors::msg &Error,
					  const SecurityObjects::UserInfo &UInfo);
		bool StartSim(const std::string &RunningId, const std::string &SimId, RESTAPI::Errors::msg &Error,
					  const SecurityObjects::UserInfo &UInfo, const std::string &MasterURI,
					  const std::string &AccessKey, std::uint64_t Offset, std::uint64_t Limit,
					  std::uint64_t Index);

		bool StopSim(const std::string &Id, RESTAPI::Errors::msg &Error, const SecurityObjects::UserInfo &UInfo);
		bool CancelSim(const std::string &Id, RESTAPI::Errors::msg &Error, const SecurityObjects::UserInfo &UInfo);

		[[nodiscard]] inline bool GetSimulationInfo( OWLSObjects::SimulationDetails & Details , const std::string &uuid = "" ) {
            std::lock_guard G(Mutex_);

            if(Simulations_.empty())
                return false;
            if(uuid.empty()) {
                Details = Simulations_.begin()->second->Details;
                return true;
            }
            auto sim_hint = Simulations_.find(uuid);
            if(sim_hint==end(Simulations_))
                return false;
			Details = sim_hint->second->Details;
            return true;
		}

		[[nodiscard]] inline const std::string &GetCasLocation() { return CASLocation_; }
		[[nodiscard]] inline const std::string &GetCertFileName() { return CertFileName_; }
		[[nodiscard]] inline const std::string &GetKeyFileName() { return KeyFileName_; }
		[[nodiscard]] inline const std::string &GetRootCAFileName() { return RootCAFileName_; }
		[[nodiscard]] inline int GetLevel() const { return Level_; }

        [[nodiscard]] static Poco::JSON::Object::Ptr GetSimConfigurationPtr(uint64_t uuid);
        [[nodiscard]] static Poco::JSON::Object::Ptr GetSimCapabilitiesPtr();
        bool IsSimulationRunning(const std::string &id);
		const auto & Services() const { return Services_; }

		void CancelRemoteSimulation(const std::string &id);
		void StopRemoteSimulation(const std::string &id);

	  private:
		Poco::Thread Worker_;
		std::atomic_bool Running_ = false;
		std::map<std::string,std::shared_ptr<SimulationRecord>> Simulations_;
		std::string CASLocation_;
		std::string CertFileName_;
		std::string KeyFileName_;
		std::string RootCAFileName_;
		Poco::JSON::Object::Ptr DefaultCapabilities_;
		int Level_ = 0;
		Types::MicroServiceMetaVec Services_;
		std::atomic_bool LookForServices_{true};
		SimulationCoordinator() noexcept
			: SubSystemServer("SimulationCoordinator", "SIM-COORDINATOR", "coordinator") {}

		void StopSimulations();
		void CancelSimulations();
	};

	inline auto SimulationCoordinator() {
		return SimulationCoordinator::instance();
	}
} // namespace OpenWifi
