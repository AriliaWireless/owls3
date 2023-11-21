//
// Created by stephane bourque on 2021-03-13.
//
#include <random>
#include <thread>
#include <chrono>

#include <fmt/format.h>

#include <Poco/Logger.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/SSLException.h>
#include <Poco/NObserver.h>

#include "SimStats.h"
#include "SimulationRunner.h"
#include "UI_Owls_WebSocketNotifications.h"
#include "Daemon.h"
#include "SimulationCoordinator.h"
#include "framework/OpenWifiTypes.h"

#include "framework/OpenAPIRequests.h"

namespace OpenWifi {

	static Poco::Net::HTTPServerResponse::HTTPStatus
	PostAPI(const Types::MicroServiceMeta &Service,
			const std::vector<std::pair<std::string,std::string>> &QueryData,
			const std::string &EndPoint, std::uint64_t TimeOut,
			Poco::JSON::Object::Ptr &ResponseObject) {

		try {
			Poco::URI URI(Service.PrivateEndPoint);
			auto Secure = (URI.getScheme() == "https");

			URI.setPath(EndPoint);
			for (const auto &qp : QueryData)
				URI.addQueryParameter(qp.first, qp.second);

			std::string Path(URI.getPathAndQuery());

			Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_POST, Path,
										   Poco::Net::HTTPMessage::HTTP_1_1);

			Request.setContentType("application/json");
			Request.setContentLength(0);

			Request.add("X-API-KEY", Service.AccessKey);
			Request.add("X-INTERNAL-NAME", MicroServicePublicEndPoint());

			if (Secure) {
				Poco::Net::HTTPSClientSession Session(URI.getHost(), URI.getPort());
				Session.setTimeout(Poco::Timespan(TimeOut / 1000, TimeOut % 1000));
				Session.sendRequest(Request);

				Poco::Net::HTTPResponse Response;
				std::istream &is = Session.receiveResponse(Response);
				if (Response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
					Poco::JSON::Parser P;
					ResponseObject = P.parse(is).extract<Poco::JSON::Object::Ptr>();
				} else {
					Poco::JSON::Parser P;
					ResponseObject = P.parse(is).extract<Poco::JSON::Object::Ptr>();
				}
				return Response.getStatus();
			} else {
				Poco::Net::HTTPClientSession Session(URI.getHost(), URI.getPort());
				Session.setTimeout(Poco::Timespan(TimeOut / 1000, TimeOut % 1000));
				Session.sendRequest(Request);

				Poco::Net::HTTPResponse Response;
				std::istream &is = Session.receiveResponse(Response);
				if (Response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
					Poco::JSON::Parser P;
					ResponseObject = P.parse(is).extract<Poco::JSON::Object::Ptr>();
				} else {
					Poco::JSON::Parser P;
					ResponseObject = P.parse(is).extract<Poco::JSON::Object::Ptr>();
				}
				return Response.getStatus();
			}
		} catch (const Poco::Exception &E) {
			Poco::Logger::get("REST-CALLER-POST").log(E);
		}
		return Poco::Net::HTTPServerResponse::HTTP_GATEWAY_TIMEOUT;
	}

	static bool StartRemoteSimulation(const Types::MicroServiceMeta &Service,
							   const std::string &RunningId, const std::string &SimulationId,
							   std::uint64_t Offset, std::uint64_t Limit, std::uint64_t Index ) {

		Poco::JSON::Object::Ptr ResponseObject;
		auto Result = PostAPI(Service,
							  {
								  { "operation", "start" },
								  { "joinRunningId", RunningId },
								  { "masterURI", MicroServicePrivateEndPoint() },
								  { "offset", std::to_string(Offset) },
								  { "limit", std::to_string(Limit) },
								  { "index", std::to_string(Index) }
							  },
							  fmt::format("/api/v1/operation/{}", SimulationId),
							  60000,
							  ResponseObject);

		std::cout << "Result: " ;
	 	ResponseObject->stringify(std::cout);
		std::cout << std::endl;

		return Result == Poco::Net::HTTPServerResponse::HTTP_OK;
	}

	 bool SimulationRunner::UpdateMasterSimulation() {
		OWLSObjects::SimulationStatus S;
		SimStats()->GetCurrent(RunningId_, S, UInfo_);
		std::uint64_t 	TimeOut=60000;
		std::cout << "UpdateMasterSimulation:" 	<< Index_ << std::endl;
		try {
			Poco::URI URI(MasterURI_);
			Poco::JSON::Object::Ptr ResponseObject;
			auto Secure = (URI.getScheme() == "https");

			URI.setPath(fmt::format("/api/v1/status/{}", RunningId_));

			std::string Path(URI.getPathAndQuery());

			Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_PUT, Path,
										   Poco::Net::HTTPMessage::HTTP_1_1);

			Request.setContentType("application/json");
			Request.setContentLength(0);

			Request.add("X-API-KEY", AccessKey_);
			Request.add("X-INTERNAL-NAME", MicroServicePublicEndPoint());

			if (Secure) {
				Poco::Net::HTTPSClientSession Session(URI.getHost(), URI.getPort());
				Session.setTimeout(Poco::Timespan(TimeOut / 1000, TimeOut % 1000));
				auto &Body = Session.sendRequest(Request);
				Poco::JSON::Object BodyObject;
				S.to_json(BodyObject);
				Poco::JSON::Stringifier::stringify(S, Body);

				Poco::Net::HTTPResponse Response;
				std::istream &is = Session.receiveResponse(Response);
				if (Response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
					Poco::JSON::Parser P;
					ResponseObject = P.parse(is).extract<Poco::JSON::Object::Ptr>();
				} else {
					Poco::JSON::Parser P;
					ResponseObject = P.parse(is).extract<Poco::JSON::Object::Ptr>();
				}
				return Response.getStatus()==Poco::Net::HTTPServerResponse::HTTP_OK;
			} else {
				Poco::Net::HTTPClientSession Session(URI.getHost(), URI.getPort());
				Session.setTimeout(Poco::Timespan(TimeOut / 1000, TimeOut % 1000));
				Session.sendRequest(Request);

				Poco::Net::HTTPResponse Response;
				std::istream &is = Session.receiveResponse(Response);
				if (Response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
					Poco::JSON::Parser P;
					ResponseObject = P.parse(is).extract<Poco::JSON::Object::Ptr>();
				} else {
					Poco::JSON::Parser P;
					ResponseObject = P.parse(is).extract<Poco::JSON::Object::Ptr>();
				}
				return Response.getStatus()==Poco::Net::HTTPServerResponse::HTTP_OK;
			}
		} catch (const Poco::Exception &E) {
			Poco::Logger::get("REST-CALLER-POST").log(E);
		}
		return Poco::Net::HTTPServerResponse::HTTP_GATEWAY_TIMEOUT;

		return true;
	}

	void SimulationRunner::Start() {
		Worker_.start(*this);
	}

	void SimulationRunner::run() {

		Logger_.information(fmt::format("Starting simulation {} with {} devices", RunningId_, Details_.devices));
		std::random_device rd;
		std::mt19937 gen(rd());

        Running_ = true;
		std::lock_guard Lock(Mutex_);

        NumberOfReactors_ = Poco::Environment::processorCount() * 2;
        for(std::uint64_t  i=0;i<NumberOfReactors_;i++) {
            auto NewReactor = std::make_unique<Poco::Net::SocketReactor>();
            auto NewReactorThread = std::make_unique<Poco::Thread>();
            NewReactorThread->start(*NewReactor);
            SocketReactorPool_.push_back(std::move(NewReactor));
            SocketReactorThreadPool_.push_back(std::move(NewReactorThread));
        }

		if(Daemon()->Master() && !SimulationCoordinator()->Services().empty()) {
			std::uint64_t BatchSize = Details_.devices / (SimulationCoordinator()->Services().size()+1);

			// we are rounding this so we share the load equally. This could mena removing a few devices.
			Details_.devices = BatchSize * (SimulationCoordinator()->Services().size()+1);

			auto Pad = Details_.devices % (BatchSize *(SimulationCoordinator()->Services().size()+1));

			Logger_.information(fmt::format("Starting multi-OWLS simulation {} with {} devices, batch size: {}", RunningId_, Details_.devices, BatchSize));

			std::uniform_int_distribution<> distrib(5, 5 * (2+((int)BatchSize / 100 )));

			std::uint64_t ReactorIndex=0;
			Logger_.information(fmt::format("Starting multi-OWLS: master with devices {} to {} , batch size: {}", 0, Details_.devices, BatchSize));
			for (uint64_t DeviceNumber = 0; DeviceNumber < BatchSize+Pad; DeviceNumber++) {
				char Buffer[32];
				snprintf(Buffer, sizeof(Buffer), "%s%05x0", Details_.macPrefix.c_str(), (unsigned int)DeviceNumber);
				auto Client = std::make_shared<OWLSclient>(Buffer, Logger_, this, *SocketReactorPool_[ReactorIndex++ % NumberOfReactors_]);
				Client->SerialNumber_ = Buffer;
				Client->Valid_ = true;
				Scheduler_.in(std::chrono::seconds(distrib(gen)), OWLSClientEvents::EstablishConnection, Client, this);
				Clients_[Buffer] = Client;
			}

			std::uint64_t Index=1;
			std::uint64_t StartValue = BatchSize+Pad;
			for(const auto & Service: SimulationCoordinator()->Services()) {
				StartRemoteSimulation(Service, RunningId_, Details_.id, StartValue, std::min(BatchSize, Details_.devices-StartValue), Index++);
				StartValue += BatchSize;
			}

		} else {
			std::uint64_t ReactorIndex=0;
			if(Daemon()->Master()) {
				Offset_ = 0 ;
				Limit_ = Details_.devices;
			}

			std::uniform_int_distribution<> distrib(5, 5 * (2+((int)Limit_ / 100 )));

			Logger_.information(fmt::format("Starting OWLS simulation {} with {} devices", RunningId_, Details_.devices));
			for (uint64_t DeviceNumber = Offset_; DeviceNumber < Limit_; DeviceNumber++) {
				char Buffer[32];
				snprintf(Buffer, sizeof(Buffer), "%s%05x0", Details_.macPrefix.c_str(), (unsigned int)DeviceNumber);
				auto Client = std::make_shared<OWLSclient>(Buffer, Logger_, this, *SocketReactorPool_[ReactorIndex++ % NumberOfReactors_]);
				Client->SerialNumber_ = Buffer;
				Client->Valid_ = true;
				Clients_[Buffer] = Client;
				Scheduler_.in(std::chrono::seconds(distrib(gen)), OWLSClientEvents::EstablishConnection, Client, this);
			}
		}

        UpdateTimerCallback_ = std::make_unique<Poco::TimerCallback<SimulationRunner>>(
                *this, &SimulationRunner::onUpdateTimer);
        UpdateTimer_.setStartInterval(10000);
        UpdateTimer_.setPeriodicInterval(2 * 1000);
        UpdateTimer_.start(*UpdateTimerCallback_, MicroServiceTimerPool());

		Logger_.information(fmt::format("Simulation {} running", RunningId_));
	}

    void SimulationRunner::onUpdateTimer([[maybe_unused]] Poco::Timer &timer) {
        if(Running_) {

			if (!Daemon()->Master()) {
				UpdateMasterSimulation();
			} else {
				OWLSNotifications::SimulationUpdate_t Notification;
				SimStats()->GetCurrent(RunningId_, Notification.content, UInfo_);
				OWLSNotifications::SimulationUpdate(Notification);
			}
            ++StatsUpdates_;

            if((StatsUpdates_ % 15) == 0) {
                std::lock_guard Lock(Mutex_);

                for(auto &client:Clients_) {
                    if(!Running_) {
                        return;
                    }
                    if(client.second->Mutex_.try_lock()) {
                        if (client.second->Connected_) {
                            client.second->Update();
                        }
                        client.second->Mutex_.unlock();
                    }
                }
            }
        }
    }

    void SimulationRunner::ProgressUpdate(SimulationRunner *sim) {
        if(sim->Running_) {
            OWLSNotifications::SimulationUpdate_t Notification;
            SimStats()->GetCurrent(sim->RunningId_, Notification.content, sim->UInfo_);
            OWLSNotifications::SimulationUpdate(Notification);
            // sim->Scheduler_.in(std::chrono::seconds(10), ProgressUpdate, sim);
        }
    }

	void SimulationRunner::Stop() {
		if (Running_) {
            Running_ = false;
            UpdateTimer_.stop();
            std::lock_guard Guard(Mutex_);
            std::for_each(SocketReactorPool_.begin(),SocketReactorPool_.end(),[](auto &reactor) { reactor->stop(); });
            std::for_each(SocketReactorThreadPool_.begin(),SocketReactorThreadPool_.end(),[](auto &t){ t->join(); });
            SocketReactorThreadPool_.clear();
            SocketReactorPool_.clear();
            Clients_.clear();
		}
	}

    void SimulationRunner::OnSocketError(const Poco::AutoPtr<Poco::Net::ErrorNotification> &pNf) {
        std::lock_guard G(Mutex_);

        std::cout << "SimulationRunner::OnSocketError" << std::endl;

        auto socket = pNf->socket().impl()->sockfd();
        std::map<std::int64_t, std::shared_ptr<OWLSclient>>::iterator client_hint;
        std::shared_ptr<OWLSclient> client;

        {
            std::lock_guard GG(SocketFdMutex_);
            client_hint = Clients_fd_.find(socket);
            if (client_hint == end(Clients_fd_)) {
                pNf->socket().impl()->close();
                poco_warning(Logger_, fmt::format("{}: Invalid socket", socket));
                return;
            }
            client = client_hint->second;
        }

        {
            std::lock_guard Guard(client->Mutex_);
            client->Disconnect(__func__, Guard);
        }
        if (Running_) {
            OWLSClientEvents::Reconnect(client, this);
        }
    }

    void SimulationRunner::OnSocketShutdown(const Poco::AutoPtr<Poco::Net::ShutdownNotification> &pNf) {
        auto socket = pNf->socket().impl()->sockfd();
        std::shared_ptr<OWLSclient> client;
        {
            std::lock_guard G(SocketFdMutex_);
            auto client_hint = Clients_fd_.find(socket);
            if (client_hint == end(Clients_fd_)) {
                pNf->socket().impl()->close();
                poco_warning(Logger_, fmt::format("{}: Invalid socket", socket));
                return;
            }
            client = client_hint->second;
        }
        {
            std::lock_guard Guard(client->Mutex_);
            client->Disconnect(__func__ , Guard);
        }
        if(Running_)
            OWLSClientEvents::Reconnect(client,this);
    }

    void SimulationRunner::OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification> &pNf) {
        std::shared_ptr<OWLSclient> client;

        int socket;
        {
            std::lock_guard G(SocketFdMutex_);
            socket = pNf->socket().impl()->sockfd();
            auto client_hint = Clients_fd_.find(socket);
            if (client_hint == end(Clients_fd_)) {
                poco_warning(Logger_, fmt::format("{}: Invalid socket", socket));
                return;
            }
            client = client_hint->second;
        }

        std::lock_guard Guard(client->Mutex_);

        try {
            Poco::Buffer<char> IncomingFrame(0);
            int Flags;

            auto MessageSize = client->WS_->receiveFrame(IncomingFrame, Flags);
            auto Op = Flags & Poco::Net::WebSocket::FRAME_OP_BITMASK;

            if (MessageSize == 0 && Flags == 0 && Op == 0) {
                OWLSClientEvents::Disconnect(__func__, Guard, client, this, "Error while waiting for data in WebSocket", true);
                return;
            }
            IncomingFrame.append(0);

            switch (Op) {
                case Poco::Net::WebSocket::FRAME_OP_PING: {
                    client->WS_->sendFrame("", 0,
                                   Poco::Net::WebSocket::FRAME_OP_PONG |
                                   Poco::Net::WebSocket::FRAME_FLAG_FIN);
                } break;

                case Poco::Net::WebSocket::FRAME_OP_PONG: {
                } break;

                case Poco::Net::WebSocket::FRAME_OP_TEXT: {
                    if (MessageSize > 0) {
                        SimStats()->AddInMsg(RunningId_, MessageSize);
                        Poco::JSON::Parser  parser;
                        auto Frame = parser.parse(IncomingFrame.begin()).extract<Poco::JSON::Object::Ptr>();

                        if (Frame->has("jsonrpc") && Frame->has("id") &&
                            Frame->has("method") && Frame->has("params")) {
                            ProcessCommand(Guard,client, Frame);
                        } else {
                            Logger_.warning(
                                    fmt::format("MESSAGE({}): invalid incoming message.", client->SerialNumber_));
                        }
                    }
                } break;
                default: {
                } break;
            }
            return;
        } catch (const Poco::Net::SSLException &E) {
            Logger_.warning(
                    fmt::format("Exception({}): SSL exception: {}", client->SerialNumber_, E.displayText()));
        } catch (const Poco::Exception &E) {
            Logger_.warning(fmt::format("Exception({}): Generic exception: {}", client->SerialNumber_,
                                        E.displayText()));
        }
        OWLSClientEvents::Disconnect(__func__, Guard,client, this, "Error while waiting for data in WebSocket", true);
    }

    void SimulationRunner::ProcessCommand(std::lock_guard<std::mutex> &ClientGuard, const std::shared_ptr<OWLSclient> & Client, Poco::JSON::Object::Ptr Frame) {

        std::string Method = Frame->get("method");
        std::uint64_t Id = Frame->get("id");
        auto Params = Frame->getObject("params");

        if (Method == "configure") {
            CensusReport_.ev_configure++;
            std::thread     t(OWLSclient::DoConfigure,Client, Id, Params);
            t.detach();
        } else if (Method == "reboot") {
            CensusReport_.ev_reboot++;
            std::thread     t(OWLSclient::DoReboot, Client, Id, Params);
            t.detach();
        } else if (Method == "upgrade") {
            CensusReport_.ev_firmwareupgrade++;
            std::thread     t(OWLSclient::DoUpgrade, Client, Id, Params);
            t.detach();
        } else if (Method == "factory") {
            CensusReport_.ev_factory++;
            std::thread     t(OWLSclient::DoFactory, Client, Id, Params);
            t.detach();
        } else if (Method == "leds") {
            CensusReport_.ev_leds++;
            std::thread     t(OWLSclient::DoLEDs, Client, Id, Params);
            t.detach();
        } else {
            Logger_.warning(fmt::format("COMMAND({}): unknown method '{}'", Client->SerialNumber_, Method));
            Client->UnSupportedCommand(ClientGuard,Client, Id, Method);
        }

    }

} // namespace OpenWifi