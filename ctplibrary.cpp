#include "stdafx.h"
#include "ctplibrary.h"
#include <iostream>

#include "rapidjson/rapidjson.h"  
#include "rapidjson/document.h"  
#include "rapidjson/reader.h"  
#include "rapidjson/writer.h"  
#include "rapidjson/stringbuffer.h"  
using namespace rapidjson;

#include ".\ThostTraderApi\ThostFtdcMdApi.h"
#include ".\ThostTraderApi\ThostFtdcTraderApi.h"
#include "MdSpi.h"
#include "TraderSpi.h"
#include "TradeInfo.h"

//https://github.com/Tencent/rapidjson


using namespace std;

// UserApi����
CThostFtdcMdApi* pMarketApi;
// �г��ӿڶ���
CThostFtdcMdSpi* pMarketSpi;

CThostFtdcTraderApi* pTraderApi;
CTraderSpi* pTraderSpi;


// ���ò���
char GlobalMarketAddr[128] = "tcp://180.168.212.228:41213";
char GlobalTradeAddr[128] = "tcp://180.168.212.228:41205";
TThostFtdcBrokerIDType	BROKER_ID = "8080";				// ���͹�˾����
TThostFtdcInvestorIDType INVESTOR_ID = "";			// Ͷ���ߴ���
TThostFtdcPasswordType  PASSWORD = "";			// �û�����
char *test[] = { (char*)"CF809", (char*)"CF901" };
char *GlobalInstruments[1024];
int GlobalInstrumentsNum = 0;									// ���鶩������
int iRequestID = 0;										// ������


// �����Ϣ
CTradeInfo* gTradeInfo;


bool InitMarket() {

	info("InitMarket()", "ENTER");
	// ��ʼ��UserApi
	if (pMarketApi == NULL) {
		if (gTradeInfo == NULL) {
			gTradeInfo = new CTradeInfo(NULL);
		}

		pMarketApi = CThostFtdcMdApi::CreateFtdcMdApi();			// ����UserApi
		pMarketSpi = new CMdSpi();
		pMarketApi->RegisterSpi(pMarketSpi);						// ע���¼���
		pMarketApi->RegisterFront(GlobalMarketAddr);					// connect
		pMarketApi->Init();

		pMarketApi->Join();
		//	pMarketApi->Release();

		return true;
	}

	return false;

}

bool InitTrade() {

	info("InitTrade()", "ENTER");

	if (pTraderApi == NULL) {
		if (gTradeInfo == NULL) {
			gTradeInfo = new CTradeInfo(NULL);
		}

		pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi();			// ����UserApi
		pTraderSpi = new CTraderSpi();
		pTraderApi->RegisterSpi((CThostFtdcTraderSpi*)pTraderSpi);			// ע���¼���
		pTraderApi->SubscribePublicTopic(THOST_TERT_QUICK);				// ע�ṫ����
		pTraderApi->SubscribePrivateTopic(THOST_TERT_QUICK);				// ע��˽����
		pTraderApi->RegisterFront(GlobalTradeAddr);							// connect
		pTraderApi->Init();

		pTraderApi->Join();

		return true;
	}

	return false;

}

bool CloseMarket() {
	cout << "Enter:Close()" << endl;

	if (pMarketApi != NULL) {
		pMarketApi->Release();
		pMarketApi = NULL;
	}

	if (pMarketSpi != NULL) {
		delete pMarketSpi;
		pMarketSpi = NULL;
	}

	return true;
}

bool CloseTrade() {

	if (pTraderApi != NULL) {
		pTraderApi->Release();
		pTraderApi = NULL;
	}

	if (pTraderSpi != NULL) {
		delete pTraderSpi;
		pTraderSpi = NULL;
	}

	return true;
}

bool Config(char *config) {
	/*
		marketaddr:
		tradeaddr:
		broker:
		investor:
		password:
		instruments:
	*/
	if (config == NULL) {
		info("Config():", "Invalid Config");
		return false;
	}

	Document doc;
	doc.Parse(config);

	Value& marketaddr = doc["marketaddr"];
	if (marketaddr != NULL) {
		const char * addr = marketaddr.GetString();
		memset(GlobalMarketAddr, 0, 128);
		memcpy(GlobalMarketAddr, addr, strlen(addr));
	}

	info("�����г�����IP��ַ:", GlobalMarketAddr);

	Value& tradeaddr = doc["tradeaddr"];
	if (tradeaddr != NULL) {
		const char *addr = tradeaddr.GetString();
		memset(GlobalTradeAddr, 0, 128);
		memcpy(GlobalTradeAddr, addr, strlen(addr));
	}

	info("���ý���IP��ַ:", GlobalTradeAddr);

	Value& valueBroker = doc["broker"];
	if (valueBroker != NULL) {
		const char *broker = valueBroker.GetString();
		memset(BROKER_ID, 0, 11);
		memcpy(BROKER_ID, broker, strlen(broker));
	}

	info("Broker:", BROKER_ID);

	Value& valueInvestor = doc["investor"];
	if (valueInvestor != NULL) {
		const char *investor = valueInvestor.GetString();
		memset(INVESTOR_ID, 0, 13);
		memcpy(INVESTOR_ID, investor, strlen(investor));
	}

	info("INVESTOR_ID:", INVESTOR_ID);

	Value& valuePwd = doc["password"];
	if (valuePwd != NULL) {
		const char *pwd = valuePwd.GetString();
		memset(PASSWORD, 0, 41);
		memcpy(PASSWORD, pwd, strlen(pwd));
	}

	cout << "Password:" << PASSWORD[0] << "******" << PASSWORD[strlen(PASSWORD)-1] << endl;
	if (!strcmp(PASSWORD, "")) {
		error("", "��Ч����");
		return false;
	}

	Value& instruments = doc["instruments"];
	if (instruments != NULL && instruments.IsArray()) {

		GlobalInstrumentsNum = 0;
		memset(GlobalInstruments, 0, sizeof(char*) * 1024);

		int length = instruments.Size();

		for (int i = 0; i < instruments.Size(); i++) {
			Value & v = instruments[i];
			assert(v.IsObject());
			if (v.HasMember("name") && v["name"].IsString()) {
				const char *name = v["name"].GetString();
				int size = strlen(name)+1;

				GlobalInstruments[i] = new char[16];
				memset(GlobalInstruments[i], 0, 16);
				memcpy(GlobalInstruments[i], name, strlen(name));

				GlobalInstrumentsNum++;
			}
		}
	}
	else {
		info("Config():", "��Ч����Ʒ����");
	}



	return true;
}

int GetDepth(char *name, char *value) {

	if (gTradeInfo != NULL) {
		return gTradeInfo->getDepth(name, value);
	}

	error("GetDepth", "gTradeInfo is NULL");
	return 0;
}

int GetInstrumentInfo(char *name, char *info) {
	int counter = 0;
	if (gTradeInfo != NULL) {
		pTraderSpi->ReqQryInstrument(name);
		while (gTradeInfo->getStatus() == StatusProcess) {

			Sleep(100);
			if (counter < 50) {
				counter++;
			}
			else {
				cout << "GetInstrumentInfo()��ʱ..." << endl;
				return 0;
			}
		}
		return gTradeInfo->getInstrumentInfo(name, info);
	}
	error("GetInstrumentInfo", "gTradeInfo is NULL");
	return 0;
}

int GetBalance(char *info) {
	int counter = 0;
	if (gTradeInfo != NULL) {
		pTraderSpi->ReqQryTradingAccount();
		while (gTradeInfo->getStatus() == StatusProcess) {

			Sleep(100);
			if (counter < 50) {
				counter++;
			}
			else {
				cout << "GetBalance()��ʱ..." << endl;
				return 0;
			}
		}
		return gTradeInfo->getAccountInfo(info);

	}
	return 0;
}



int Test(char *echo) {
	cout << "Test:" << echo << endl;

	const char* str = "{\"name\":\"xiaoming\",\"age\":18,\"job\":\"coder\",\"a\":{\"b\":1}}";

	Document doc;
	doc.Parse(str);

	Value& s = doc["age"];
	s.SetInt(s.GetInt() + 1);

	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	doc.Accept(writer);

	const char * result = buffer.GetString();
	cout << "String:" << result << endl;

	memcpy(echo, result, strlen(result));

	return strlen(result);
}

void info(const char *prefix, const char *msg) {
	cout << "[INFO]"<< prefix << msg << endl;
}
void debug(const char *prefix, const char *msg) {
	cout << "[DEBUG]" << prefix << msg << endl;
}

void error(const char *prefix, const char *msg) {
	cout << "[ERROR]" << prefix << msg << endl;
}