/*!
Main class
*/
#ifndef SMQTTBROKER_FILE
#define SMQTTBROKER_FILE

#include"sMQTTplatform.h"
#include"sMQTTClient.h"
#include"sMQTTTopic.h"
#include"sMQTTEvent.h"

class sMQTTBroker
{
public:
	/*! setup broker \param port set listen port \param checkWifiConnection enable/disable notify wifi connection in onEvent*/
	bool init(unsigned short port, bool checkWifiConnection=false);
	void setMaxClients(size_t limit) { maxClients = limit; }
    size_t getClientCount() const { return clients.size(); }
    size_t getConnectedClientCount() {
        size_t count = 0;
        for (auto *client : clients) if (client->isAuthenticated() && client->isConnected()) ++count;
        return count;
    }
    unsigned long getRejectedClientCount() const { return rejectedClients; }
    void shutdown();
	/*! call in loop function*/
	void update();
	/*! publish message
		\param topic name of topic
		\param payload
		\param qos
		\param retain
	*/
	void publish(const std::string &topic, const std::string &payload,unsigned char qos=0,bool retain=false);
	//! restart WIFI server
	void restart();
	//! receive event from broker \param event
	virtual bool onEvent(sMQTTEvent *event) = 0;

	//! receive retained topic count
	unsigned long getRetainedTopicCount();
	//! receive topic name by index \param index index of topic
	std::string getRetaiedTopicName(unsigned long index);
private:
	// inner function
	void publish(sMQTTClient *client, sMQTTTopic *topic, sMQTTMessage *msg);

	bool subscribe(sMQTTClient *client, const char *topic);
	void unsubscribe(sMQTTClient *client, const char *topic);

	bool isTopicValidName(const char *filter);
	void updateRetainedTopic(sMQTTTopic *topic);

	bool isClientConnected(sMQTTClient *client);

	void findRetainTopic(sMQTTTopic *topic, sMQTTClient *client);

	TCPServer *_server = nullptr;
	size_t maxClients = 30;
	unsigned long rejectedClients = 0;
	sMQTTClientList clients;
	sMQTTTopicList subscribes, retains;
	bool isCheckWifiConnection;
	friend class sMQTTClient;
};

class sMQTTBrokerWithoutEvent:public sMQTTBroker
{
public:
	bool onEvent(sMQTTEvent *event) { return true; }
};
#endif