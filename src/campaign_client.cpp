#include "engine.h"
#include "campaign_client.h"
#include "multiplayer_server.h"
#include "multiplayer_proxy.h"
#include <cctype>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>

P<CampaignClient> campaign_client;

CampaignClient::CampaignClient(string hostname, int port): campaign_server_hostname(hostname), campaign_server_port(port)
{
    campaign_client = this;
}

bool CampaignClient::isOnline() {
    auto hostname = campaign_server_hostname;
    auto port = campaign_server_port;
    auto ping = httpGetJson("/");
    if(!ping.is_discarded() && ping["message"] == "Hello Space"){
            LOG(INFO) << "Connected to Campaign Server " << hostname << ":" << port;
            return true;
    } else {
            LOG(WARNING) << "Connection to Campaign Server " << hostname << ":" << port << " failed: " << ping.dump();
            return false;
    }
}

string CampaignClient::httpRequest(const string& path, const string& body, bool post)
{
    sp::io::http::Request request(campaign_server_hostname, campaign_server_port);
    request.setHeader("Content-Type", "application/json");
    
    LOG(INFO) << "Sending Http request: " << campaign_server_hostname << ":" << campaign_server_port << path;

    sp::io::http::Request::Response response;
    if (post) {
        LOG(DEBUG) << "POST " << body;
        response = request.post(path, body);
    }
    else
    {
        response = request.get(path);
    }
    // warning: this will block until response is received
    // start this function in a thread to avoid blocking
    if (!response.success)
    {
        LOG(WARNING) << "Http request failed. (status " << response.status << ")";
        LOG(WARNING) << body;
        return "";
    }else{
        return response.body;
    }
    return "";
}

void CampaignClient::httpGetNoResponse(string path)
{
    std::thread(&CampaignClient::httpRequest, this, path, "", false).detach();
}

void CampaignClient::httpPostNoResponse(string path, string body)
{
    std::thread(&CampaignClient::httpRequest, this, path, body, true).detach();
}

// You need to store the future somewhere and call .get().
std::future<string> CampaignClient::httpGet(string path)
{
    return std::async(std::launch::async, &CampaignClient::httpRequest, this, path, "", false);
}

std::future<string> CampaignClient::httpPost(string path, string body)
{
    return std::async(std::launch::async, &CampaignClient::httpRequest, this, path, body, true);
}

nlohmann::json CampaignClient::httpGetJson(string path)
{
    auto result_body = httpGet(path);
    result_body.wait();
    auto result_string = result_body.get();
    auto result_json = nlohmann::json::parse(result_string, nullptr, false);
    if (result_json.is_discarded())
    {
        LOG(ERROR) << "JSON-ERROR: can not parse " << result_string;
        LOG(ERROR) << result_json.dump();
    }
    return result_json;
}

void CampaignClient::notifyCampaignServer(string event, nlohmann::json scenario_info)
{
    nlohmann::json body = {
        {"scenario_info", scenario_info},
        {"server_name", urlencode(getServerName())},
    };
    string path = "/"+event;
    httpPostNoResponse(path.c_str(), body.dump());
}

std::vector<string> CampaignClient::getScenarios()
{
    string path = string("/scenarios/")+urlencode(getServerName());
    LOG(INFO) << "loading scenarios from campaign server";
    auto result_json = httpGetJson(path);
    auto result_array = result_json["scenarios"];
    LOG(DEBUG) << result_array.dump();
    std::vector<string> entries;
    for (auto scenario : result_array)
    {
        entries.push_back(scenario.get<std::string>());
    }
    return entries;
}

std::map<string, string> CampaignClient::getScenarioInfo(string name)
{
    string path = string("/scenario_info/")+urlencode(getServerName())+"/"+urlencode(name);
    auto result_json = httpGetJson(path);
    auto result_map = result_json["scenarioInfo"];
    std::map<string, string> kvpairs;
    for (auto& [key, value] : result_map.items()){
        kvpairs[key] = value.get<std::string>();
    }
    return kvpairs;
}

std::map<string, std::vector<string> > CampaignClient::getScenarioSettings(string name)
{
    string path = string("/scenario_settings/")+urlencode(getServerName())+"/"+urlencode(name);
    auto result_json = httpGetJson(path);
    LOG(INFO) << result_json.dump();
    std::map<string, std::vector<string> > settings;
    for (auto& [setting, options] : result_json.items()){
        std::vector<string> entries;
        for (auto value: options)
        {
            entries.push_back(value.get<std::string>());
        }
        settings[setting] = entries;
    }
    return settings;
}

std::vector<string> CampaignClient::getShips()
{
    string path = string("/ships_available/")+urlencode(getServerName());
    auto result_json = httpGetJson(path);
    auto result_array = result_json["ships"];
    std::vector<string> elements;
    for (auto& element : result_array){
        elements.push_back(element.get<std::string>());
    }
    return elements;
}

void CampaignClient::spawnShipOnProxy(string server_ip, string ship_name, string ship_template, string drive, string ship_password, int x, int y){
    nlohmann::json body = {
        {"server_ip", server_ip},
        {"callsign", ship_name},
        {"template", ship_template},
        {"drive", drive},
        {"password", ship_password},
        {"x", x},
        {"y", y},
    };
    string uri = "/proxySpawn";
    httpPostNoResponse(uri.c_str(), body.dump());
}

void CampaignClient::destroyShipOnProxy(string server_ip, string ship_name){
    nlohmann::json body = {
        {"server_ip", server_ip},
        {"callsign", ship_name},
    };
    string uri = "/proxyDestroy";
    httpPostNoResponse(uri.c_str(), body.dump());
}
const string CampaignClient::getServerName(){
    string name = "unknown";
    if (game_server) {
        name = game_server->getServerName();
    } else if (game_proxy) {
        name = game_proxy->getProxyName();
    }
    return name;
}

const string CampaignClient::urlencode(const string& value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

