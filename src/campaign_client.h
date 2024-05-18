#ifndef CAMPAIGN_CLIENT_H 
#define CAMPAIGN_CLIENT_H 

#include <stdint.h>
#include <thread>
#include <future>
#include <map>
#include <vector>
#include "io/network/udpSocket.h"
#include "io/network/tcpSocket.h"
#include "io/network/streamSocket.h"
#include "io/network/tcpListener.h"
#include "Updatable.h"
#include "stringImproved.h"
#include <io/json.h>
#include <io/http/request.h>

class CampaignClient;

extern P<CampaignClient> campaign_client;

class CampaignClient: public virtual PObject
{
    string campaign_server_hostname;
    int campaign_server_port;
    string instance_name;
    string instance_name_url;

public:
    CampaignClient(string hostname, int port, string instance_name);
    void setClientName(string client_name) {client_name = client_name;};
    bool isOnline();
    void notifyCampaignServer(string event, nlohmann::json info);
    void notifyCampaignServerScreen(string screen) {
        notifyCampaignServer("screen", {{"screen", screen}});
    };
    std::vector<string> getScenarios();
    nlohmann::json getCampaign();
    std::map<string, string> getScenarioInfo(string name);
    std::map<string, std::vector<string> > getScenarioSettings(string name);
    std::vector<string> getShips();
    string getBriefing();
    string getCampaignServerURL() { return campaign_server_hostname; }
    void spawnShipOnProxy(string server_ip, string ship_name, string ship_template, string drive, string ship_password, int x, int y, int rota);
    void destroyShipOnProxy(string server_ip, string ship_name);
private:
    void httpGetNoResponse(string path);
    void httpPostNoResponse(string path, string body);
    std::future<string> httpGet(string path);
    std::future<string> httpPost(string path, string body);
    nlohmann::json httpGetJson(string path);

    string httpRequest(const string& path, const string& body="", bool post = false);
    const string urlencode(const string&);
};

#endif//MULTIPLAYER_SERVER_H
