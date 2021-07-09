#include "engine.h"
#include "campaign_client.h"
#include "multiplayer_server.h"

P<CampaignClient> campaign_client;

CampaignClient::CampaignClient(string url): campaign_server_url(url)
{
	campaign_client = this;
}

string CampaignClient::httpRequest(const string& url, sf::Http::Request::Method method, const string& body)
{

    string hostname = url;
    if (url.startswith("http://"))
    {
		hostname = url.substr(7);
    }

    int path_start = hostname.find("/");
    if (path_start < 0)
    {
        LOG(ERROR) << "Http request URL " << url << " does not have a URI after the hostname";
        return "";
    }
    int port = 80;
    int port_start = hostname.find(":");
    string uri = hostname.substr(path_start + 1);
    if (port_start >= 0)
    {
        // If a port is attached to the hostname, parse it out.
        // No validation is performed.
        port = hostname.substr(port_start + 1, path_start).toInt();
        hostname = hostname.substr(0, port_start);
    }else{
        hostname = hostname.substr(0, path_start);
    }
    
    sf::Http http(hostname, static_cast<uint16_t>(port));
    sf::Http::Request request(uri, method, body);
	request.setField("Content-Type", "application/json");
    
    LOG(INFO) << "Sending Http request: " << url;
    sf::Http::Response response = http.sendRequest(request, sf::seconds(10.0f));
    // warning: this will block until response is received or timeout is reached.
    // start this function in a thread to avoid blocking
    if (response.getStatus() != sf::Http::Response::Ok)
    {
        LOG(WARNING) << "Http request failed. (status " << response.getStatus() << ")";
        LOG(WARNING) << body;
        return "";
    }else{
        return response.getBody();
    }
    return "";
}

void CampaignClient::httpGetNoResponse(string url)
{
    std::thread(&CampaignClient::httpRequest, this, url, sf::Http::Request::Get, "").detach();
}

void CampaignClient::httpPostNoResponse(string url, string body)
{
    std::thread(&CampaignClient::httpRequest, this, url, sf::Http::Request::Post, body).detach();
}

// You need to store the future somewhere and call .get().
std::future<string> CampaignClient::httpGet(string url)
{
    return std::async(std::launch::async, &CampaignClient::httpRequest, this, url, sf::Http::Request::Get, "");
}

std::future<string> CampaignClient::httpPost(string url, string body)
{
    return std::async(std::launch::async, &CampaignClient::httpRequest, this, url, sf::Http::Request::Post, body);
}

void CampaignClient::notifyCampaignServer(string event, json11::Json scenario_info)
{
    json11::Json body = json11::Json::object {
        {"server_name", game_server->getServerName().c_str()},
        {"foo", "bar"}
//        {"scenario_info", scenario_info},
    };
	string uri = campaign_server_url+"/"+event+"/";
    httpPostNoResponse(uri.c_str(), body.dump());
}

std::vector<string> CampaignClient::getScenarios()
{
    auto result_body = httpGet(campaign_server_url+"/scenarios/"+game_server->getServerName().c_str());
	string err;
	result_body.wait();
	auto result_string = result_body.get();
	auto result_json = json11::Json::parse(result_string, err);
	if (err != "")
	{
		LOG(ERROR) << err;
	}
	auto result_array = result_json["scenarios"];
	LOG(DEBUG) << result_array.dump();
	std::vector<string> entries;
	for (auto scenario : result_array.array_items())
	{
		entries.push_back(scenario.string_value());
	}
	return entries;
}

std::map<string, string> CampaignClient::getScenarioInfo(string name)
{
    auto result_body = httpGet(campaign_server_url+"/scenario_info/"+game_server->getServerName().c_str()+"/"+name.c_str());
	string err;
	result_body.wait();
	auto result_string = result_body.get();
	auto result_json = json11::Json::parse(result_string, err);
	if (err != "")
	{
		LOG(ERROR) << err;
	}
	auto result_map = result_json["scenarioInfo"];
	std::map<string, string> kvpairs;
	return kvpairs;
}
