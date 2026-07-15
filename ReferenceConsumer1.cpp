/**
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  */

/*
 * ReferenceConsumer.cpp
 *
 *  @Copyright (C) 2018, SurgiTAIX AG
 *  Author: baumeister, rosenau
 *
 *  This example demonstrates how to set up a very simple SDCConsumer. It connects to the ExampleProvider defined with it's endpoint reference (EPR).
 *  A state handler is facilitated to utilize the eventing mechanism for a numeric metric state.
 *
 */
#include <chrono>
#include <fstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <curl/curl.h>
#include "SDCLib/SDCLibrary.h"
#include "SDCLib/Data/SDC/SDCConsumer.h"
#include "SDCLib/Data/SDC/SDCConsumerConnectionLostHandler.h"
#include "SDCLib/Data/SDC/SDCConsumerMDStateHandler.h"

#include "SDCLib/Data/SDC/MDIB/NumericMetricState.h"
#include "SDCLib/Data/SDC/MDIB/NumericMetricValue.h"

#include "SDCLib/Data/SDC/MDIB/StringMetricState.h"
#include "SDCLib/Data/SDC/MDIB/StringMetricValue.h"

#include "SDCLib/Util/DebugOut.h"
#include "OSELib/SDC/ServiceManager.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace SDCLib;
using namespace SDCLib::Util;
using namespace SDCLib::Data::SDC;

void updatePatientJson();

double heartRate = 0;
double spo2 = 0;
double systolicBP = 0;
double diastolicBP = 0;
std::string spo2String = "";
std::string bloodPressure = "";

std::string status = "Disconnected";

std::mutex dataMutex;

const std::string HANDLE_HEART_RATE = "numeric.ch1.vmd0";
const std::string HANDLE_SPO2 = "numeric.ch1.vmd0";
const std::string HANDLE_SYS = "numeric.ch0.vmd1";

const std::string HANDLE_BP_STRING = "string.ch1.vmd0";
const std::string HANDLE_SPO2_STRING = "spo2.ch1.vmd0";
void sendToAPI()
{
    CURL *curl = curl_easy_init();

    if(curl)
    {
        json data;

        data["status"] = status;
        data["heartRate"] = heartRate;
        data["bloodPressure"] = bloodPressure;
        data["spo2"] = spo2;


        std::string jsonData = data.dump();


        struct curl_slist *headers = NULL;

        headers = curl_slist_append(
            headers,
            "Content-Type: application/json"
        );


        curl_easy_setopt(
            curl,
            CURLOPT_URL,
"https://cpppatient-monitorbackend-production.up.railway.app/update"
        );


        curl_easy_setopt(
            curl,
            CURLOPT_HTTPHEADER,
            headers
        );


        curl_easy_setopt(
            curl,
            CURLOPT_POSTFIELDS,
            jsonData.c_str()
        );


        CURLcode res = curl_easy_perform(curl);


        if(res != CURLE_OK)
        {
            std::cout 
            << "API Error: "
            << curl_easy_strerror(res)
            << std::endl;
        }


        curl_easy_cleanup(curl);
    }
}
class NumericMetricEventHandler
    : public SDCConsumerMDStateHandler<NumericMetricState>
{
public:
    NumericMetricEventHandler(const std::string& handle)
        : SDCConsumerMDStateHandler<NumericMetricState>(handle)
    {
    }
void onStateChanged(const NumericMetricState& state) override
{
    double value = state.getMetricValue().getValue();

    std::cout 
        << getDescriptorHandle()
        << " : "
        << value
        << std::endl;


    {
        std::lock_guard<std::mutex> lock(dataMutex);

        if(getDescriptorHandle() == "numeric.ch1.vmd0")
        {
            heartRate = value;
        }

        status = "Connected";
    }

    updatePatientJson();
    sendToAPI();
}
    
};


class StringMetricEventHandler
    : public SDCConsumerMDStateHandler<StringMetricState>
{
public:
    StringMetricEventHandler(const std::string& handle)
        : SDCConsumerMDStateHandler<StringMetricState>(handle)
    {
    }
void onStateChanged(const StringMetricState& state) override
{
    std::string value = state.getMetricValue().getValue();

    std::cout
        << "STRING HANDLE = "
        << getDescriptorHandle()
        << " VALUE = "
        << value
        << std::endl;


    {
        std::lock_guard<std::mutex> lock(dataMutex);

        if(getDescriptorHandle() == HANDLE_BP_STRING)
        {
            bloodPressure = value;
        }
        else if(getDescriptorHandle() == HANDLE_SPO2_STRING)
        {
            spo2String = value;

            if(!value.empty() && value.back() == '%')
            {
                spo2 = std::stod(value.substr(0, value.size()-1));
            }
        }

        status = "Connected";
    }


    // call after releasing mutex
    updatePatientJson();
    sendToAPI();
}
    
};


auto bpHandler =
std::make_shared<StringMetricEventHandler>(HANDLE_BP_STRING);

auto spo2StringHandler =
std::make_shared<StringMetricEventHandler>(HANDLE_SPO2_STRING);


class MyConnectionLostHandler
    : public SDCConsumerConnectionLostHandler
{
public:

    MyConnectionLostHandler(SDCConsumer& consumer)
        : m_consumer(consumer)
    {
    }

    void onConnectionLost() override
    {
        std::cout << "Connection Lost" << std::endl;

        {
            std::lock_guard<std::mutex> lock(dataMutex);
            status = "Disconnected";
        }

        updatePatientJson();

        m_consumer.disconnect();
    }

private:
    SDCConsumer& m_consumer;
};


bool ends_with(const std::string& value,
               const std::string& ending)
{
    if(ending.size() > value.size())
        return false;

    return std::equal(
        ending.rbegin(),
        ending.rend(),
        value.rbegin()
    );
}

void updatePatientJson()
{
    std::lock_guard<std::mutex> lock(dataMutex);

    json patient;

    patient["status"] = status;
    patient["heartRate"] = heartRate;
    patient["bloodPressure"] = bloodPressure;
    patient["spo2"] = spo2;


    std::ofstream file("patient_data1.json");

    if(file.is_open())
    {
        file << patient.dump(4);
        file.close();
    }
}

int main(int argc, char* argv[])
{
    DebugOut::DEBUG_LEVEL = DebugOut::Silent;

    bool useTLS = false;
    std::string endpointEnding = "";

    for(int i = 0; i < argc; i++)
    {
        if(std::string(argv[i]) == "-tls")
        {
            useTLS = true;
        }

        if(std::string(argv[i]) == "-epr" && i + 1 < argc)
        {
            endpointEnding = argv[i + 1];
        }
    }

    SDCLibrary::getInstance().startup(OSELib::LogLevel::None);

    auto sdcInstance = std::make_shared<SDCInstance>(true);

    sdcInstance->setIP6enabled(false);
    sdcInstance->setIP4enabled(true);

    if(!sdcInstance->bindToDefaultNetworkInterface())
    {
        std::cout << "Failed to bind network interface." << std::endl;
        return -1;
    }

    if(useTLS)
    {
        if(!sdcInstance->initSSL())
        {
            std::cout << "Failed to initialize SSL." << std::endl;
            return -1;
        }

        auto sslConfig = sdcInstance->getSSLConfig();

        sslConfig->addCertificateAuthority("ca.pem");
        sslConfig->useCertificate("sdccert.pem");
        sslConfig->useKeyFiles("", "userkey.pem", "");
    }

    OSELib::SDC::ServiceManager serviceManager(sdcInstance);

    auto devices = serviceManager.discover();

    if(devices.empty())
    {
        std::cout << "No Provider Found." << std::endl;
        return -1;
    }

    std::unique_ptr<SDCConsumer> consumer;

    if(endpointEnding.empty())
    {
        std::cout << "Available Providers\n";

        for(size_t i = 0; i < devices.size(); i++)
        {
            std::cout
                << i + 1
                << ". "
                << devices[i]->getEndpointReference()
                << std::endl;
        }

        int choice;

        std::cout << "Select Provider : ";
        std::cin >> choice;

        consumer = std::move(devices[choice - 1]);
    }
    else
    {
        for(auto& dev : devices)
        {
            if(ends_with(dev->getEndpointReference(), endpointEnding))
            {
                consumer = std::move(dev);
                break;
            }
        }
    }

    if(!consumer)
    {
        std::cout << "Provider not selected." << std::endl;
        return -1;
    }

    MyConnectionLostHandler connectionHandler(*consumer);

    consumer->setConnectionLostHandler(&connectionHandler);

    auto heartRateHandler =
        std::make_shared<NumericMetricEventHandler>(HANDLE_HEART_RATE);

    

    auto systolicHandler =
        std::make_shared<NumericMetricEventHandler>(HANDLE_SYS);

    bool ok = true;

    ok &= consumer->registerStateEventHandler(heartRateHandler.get());

   

    ok &= consumer->registerStateEventHandler(systolicHandler.get());

    ok &= consumer->registerStateEventHandler(bpHandler.get());

    ok &= consumer->registerStateEventHandler(spo2StringHandler.get());

    if(ok)
    {
        std::cout << "Connected Successfully." << std::endl;
    }
    else
    {
        std::cout << "Failed to register handlers." << std::endl;
        return -1;
    }

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
