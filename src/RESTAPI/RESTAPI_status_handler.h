//
// Created by stephane bourque on 2021-11-02.
//

#pragma once

#include "framework/RESTAPI_Handler.h"

namespace OpenWifi {
	class RESTAPI_status_handler : public RESTAPIHandler {
	  public:
		RESTAPI_status_handler(const RESTAPIHandler::BindingMap &bindings, Poco::Logger &L,
							   RESTAPI_GenericServerAccounting &Server, uint64_t TransactionId,
							   bool Internal)
			: RESTAPIHandler(bindings, L,
							 std::vector<std::string>{Poco::Net::HTTPRequest::HTTP_GET,
													  Poco::Net::HTTPRequest::HTTP_PUT,
													  Poco::Net::HTTPRequest::HTTP_OPTIONS},
							 Server, TransactionId, Internal) {}
		static auto PathName() { return std::list<std::string>{"/api/v1/status/{id}"}; }
		void DoGet() final;
		void DoPost() final{};
		void DoPut() final;
		void DoDelete() final{};

	  private:
	};

} // namespace OpenWifi
