//
// Created by stephane bourque on 2023-11-19.
//

#pragma once

#include <iostream>
#include <Poco/Net/NetworkInterface.h>
namespace OpenWifi::SockUtils {

	inline std::string GetLocalAddress() {
		try {
			auto interfaces = Poco::Net::NetworkInterface::list();
			for (const auto &i : interfaces) {
				std::cout << i.name() << ": " << std::endl;
				std::cout << i.adapterName() << ": " << std::endl;
				if (i.supportsIPv4() && i.isUp()) {
					auto addresses = i.addressList();
					for(const auto &a:addresses) {
						std::cout << "  " << a.get<0>().toString() << std::endl;
					}
				}
			}
		} catch (const Poco::Exception &E) {
			std::cout << "Exception: " << E.displayText() << std::endl;
		}
		return "";
	}
}; // namespace OpenWifi
