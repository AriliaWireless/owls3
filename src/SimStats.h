//
// Created by stephane bourque on 2021-04-07.
//

#pragma once

#include <framework/SubSystemServer.h>
#include <framework/utils.h>
#include <RESTObjects/RESTAPI_OWLSobjects.h>
#include <RESTObjects/RESTAPI_SecurityObjects.h>
#include <SimulationCoordinator.h>
#include <Daemon.h>

namespace OpenWifi {

	class SimStats : public SubSystemServer {

	  public:
		inline void Connect(const std::string &id) {
			std::lock_guard G(Mutex_);

            auto stats_hint = Statuses_.find(id);
            if(stats_hint==end(Statuses_)) {
                return;
            }
			stats_hint->second[0].liveDevices++;
			UpdateConnections(id);
		}

		inline void UpdateConnections(const std::string &id) {
			auto stats_hint = Statuses_.find(id);
			if(stats_hint==end(Statuses_)) {
				return;
			}

			if(Daemon()->Master()) {
				std::uint64_t devices_now=0;
				std::for_each(begin(stats_hint->second), end(stats_hint->second), [&devices_now](const OWLSObjects::SimulationStatus &S) {
					devices_now += S.liveDevices;
				});
				if ((stats_hint->second[0].timeToFullDevices == 0) &&
					(stats_hint->second[0].expectedDevices == devices_now)) {
					uint64_t Now = Utils::Now();
					stats_hint->second[0].timeToFullDevices = Now - stats_hint->second[0].startTime;
				}
			}
		}

		inline void Disconnect(const std::string &id) {
            std::lock_guard G(Mutex_);

            auto stats_hint = Statuses_.find(id);
            if(stats_hint==end(Statuses_)) {
                return;
            }

            if (stats_hint->second[0].liveDevices)
                stats_hint->second[0].liveDevices--;
		}

		static auto instance() {
			static auto instance_ = new SimStats;
			return instance_;
		}

		inline void AddOutMsg(const std::string &id, int64_t N) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Statuses_.find(id);
            if(stats_hint==end(Statuses_)) {
                return;
            }
            stats_hint->second[0].msgsTx++;
            stats_hint->second[0].tx += N;
		}

		inline void AddInMsg(const std::string &id, int64_t N) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Statuses_.find(id);
            if(stats_hint==end(Statuses_)) {
                return;
            }
            stats_hint->second[0].rx += N;
            stats_hint->second[0].msgsRx++;
		}

		inline void GetCurrent(const std::string &id, OWLSObjects::SimulationStatus &Result,
                               const SecurityObjects::UserInfo & UInfo) {

			std::lock_guard G(Mutex_);
			GetCurrentNoLock(id, Result, UInfo);
		}

		inline void GetCurrentNoLock(const std::string &id, OWLSObjects::SimulationStatus &Result,
									 const SecurityObjects::UserInfo & UInfo) {
			
			auto stats_hint = Statuses_.find(id);
			if (stats_hint == end(Statuses_)) {
				return;
			}
			if (UInfo.userRole == SecurityObjects::ROOT ||
				UInfo.email == stats_hint->second[0].owner) {
				if (Daemon()->Master()) {
					Result.liveDevices = Result.rx = Result.tx = Result.msgsRx = Result.msgsTx =
						Result.errorDevices = 0;
					Result = std::accumulate(begin(stats_hint->second), end(stats_hint->second), Result,
											 [&](const OWLSObjects::SimulationStatus &A,
												 const OWLSObjects::SimulationStatus &B) {
												 OWLSObjects::SimulationStatus S;
												 S.liveDevices = A.liveDevices + B.liveDevices;
												 S.rx = A.rx + B.rx;
												 S.tx = A.tx + B.tx;
												 S.msgsRx = A.msgsRx + B.msgsRx;
												 S.msgsTx = A.msgsTx + B.msgsTx;
												 S.errorDevices = A.errorDevices + B.errorDevices;
												 return S;
											 });
					Result.state = stats_hint->second[0].state;
					Result.id = stats_hint->second[0].id;
					Result.simulationId = stats_hint->second[0].simulationId;
					Result.startTime = stats_hint->second[0].startTime;
					Result.endTime = stats_hint->second[0].endTime;
					Result.owner = stats_hint->second[0].owner;
					Result.expectedDevices = stats_hint->second[0].expectedDevices;
					Result.timeToFullDevices = stats_hint->second[0].timeToFullDevices;
				} else {
					Result = stats_hint->second[0];
				}
			}
		}

		inline int Start() final {
			return 0;
		}

		inline void Stop() final {

        }

		inline void StartSim(const std::string &id, OWLSObjects::SimulationDetails &SimDetails,
                             const SecurityObjects::UserInfo & UInfo) {
			std::lock_guard G(Mutex_);

			auto & CurrentStatus = Statuses_[id];

			OWLSObjects::SimulationStatus S;
			S.expectedDevices = SimDetails.devices;
			S.id = id;
			S.simulationId = SimDetails.id;
			S.state = "running";
			S.liveDevices = S.endTime = S.rx = S.tx = S.msgsTx =
			S.msgsRx = S.timeToFullDevices = S.errorDevices = 0;
			S.startTime = Utils::Now();
			S.owner = UInfo.email;

			if(Daemon()->Master()) {
				for(std::uint64_t i=0;i<SimulationCoordinator()->Services().size()+1;++i) {
					CurrentStatus.emplace_back(S);
				}
			} else {
				CurrentStatus.emplace_back(S);
			}
		}

		inline void EndSim(const std::string &id) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Statuses_.find(id);
            if(stats_hint==end(Statuses_)) {
                return;
            }
			stats_hint->second[0].state = "completed";
            stats_hint->second[0].endTime = Utils::Now();
		}

        inline void RemoveSim(const std::string &id) {
            std::lock_guard G(Mutex_);
            Statuses_.erase(id);
        }

		inline void SetState(const std::string &id, const std::string &S) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Statuses_.find(id);
            if(stats_hint==end(Statuses_)) {
                return;
            }
            stats_hint->second[0].state = S;
		}

		[[nodiscard]] inline std::string GetState(const std::string &id) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Statuses_.find(id);
            if(stats_hint==end(Statuses_)) {
                return "";
            }
			return stats_hint->second[0].state;
		}

		inline void Reset(const std::string &id) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Statuses_.find(id);
            if(stats_hint==end(Statuses_)) {
                return;
            }

            stats_hint->second[0].liveDevices =
            stats_hint->second[0].rx =
            stats_hint->second[0].tx =
            stats_hint->second[0].msgsRx =
            stats_hint->second[0].msgsTx =
            stats_hint->second[0].errorDevices =
            stats_hint->second[0].startTime =
            stats_hint->second[0].endTime = 0;
            stats_hint->second[0].state = "idle";
		}

		[[nodiscard]] inline uint64_t GetStartTime(const std::string &id) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Statuses_.find(id);
            if(stats_hint==end(Statuses_)) {
                return 0;
            }
            return stats_hint->second[0].startTime;
        }

/*		[[nodiscard]] inline uint64_t GetLiveDevices(const std::string &id) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Statuses_.find(id);
            if(stats_hint==end(Statuses_)) {
                return 0;
            }
            return stats_hint->second[0].liveDevices;
        }
*/
		inline void UpdateRemoteStatus(const OWLSObjects::SimulationStatus &SimStatus, std::uint64_t Index) {
			std::lock_guard G(Mutex_);
			auto stats_hint = Statuses_.find(SimStatus.id);
			if (stats_hint == end(Statuses_)) {
				return;
			}

			if(Index<stats_hint->second.size()) {
				stats_hint->second[Index] = SimStatus;
				UpdateConnections(SimStatus.id);
			} else {
				Logger().warning(fmt::format("Invalid index {} for simulation {} size {}", Index, SimStatus.id, stats_hint->second.size()));
			}
		}

        inline void GetAllSimulations(std::vector<OWLSObjects::SimulationStatus> & Statuses, const SecurityObjects::UserInfo & UInfo) {
            Statuses.clear();
            std::lock_guard G(Mutex_);
            for(const auto &[id,status]:Statuses_) {
                if(UInfo.userRole==SecurityObjects::ROOT || UInfo.email==status[0].owner) {
					OWLSObjects::SimulationStatus SimStatus;
					GetCurrentNoLock(id, SimStatus, UInfo);
					Statuses.emplace_back(SimStatus);
                }
            }
        }

	  private:
        std::map<std::string,std::vector<OWLSObjects::SimulationStatus>>     Statuses_;
		SimStats() noexcept : SubSystemServer("SimStats", "SIM-STATS", "stats") {}
	};

	inline auto SimStats() { return SimStats::instance(); }
} // namespace OpenWifi
