//
// Created by stephane bourque on 2023-11-21.
//

#pragma once

#include <Poco/JSON/Parser.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <framework/MicroService.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/URI.h>
#include <framework/MicroServiceFuncs.h>

namespace OpenWifi {
	inline  Poco::Net::HTTPServerResponse::HTTPStatus

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


}