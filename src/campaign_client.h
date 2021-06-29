#ifndef CAMPAIGN_CLIENT_H 
#define CAMPAIGN_CLIENT_H 

#include <stdint.h>
#include <thread>
#include <future>
#include <map>
#include <vector>
#include "fixedSocket.h"
#include "Updatable.h"
#include "stringImproved.h"
#include "json11/json11.hpp"

class CampaignClient;

extern P<CampaignClient> campaign_client;

class CampaignClient: public virtual PObject
{
    string campaign_server_url;

public:
	CampaignClient(string url);
    void notifyCampaignServer(string event, json11::Json scenario_info);
	std::vector<string> getScenarios();
	std::map<string, string> getScenarioInfo(string name);
	std::vector<string> getShips();
	string getCampaignServerURL() { return campaign_server_url; }
    void spawnShipOnProxy(string server_ip, string ship_name, string ship_template, string ship_password);
private:
    void httpGetNoResponse(string url);
    void httpPostNoResponse(string url, string body);
    std::future<string> httpGet(string url);
    std::future<string> httpPost(string url, string body);

    string httpRequest(const string& url, sf::Http::Request::Method method= sf::Http::Request::Get, const string& body="");
	const string urlencode(const string&);
};

#endif//MULTIPLAYER_SERVER_H
