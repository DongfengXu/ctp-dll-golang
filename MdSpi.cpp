#include "MdSpi.h"
#include <iostream>
#include "TradeInfo.h"
using namespace std;

#pragma warning(disable : 4996)

// USER_API����
extern CThostFtdcMdApi* pMarketApi;

// ���ò���	
extern TThostFtdcBrokerIDType	BROKER_ID;
extern TThostFtdcInvestorIDType INVESTOR_ID;
extern TThostFtdcPasswordType	PASSWORD;
extern char* GlobalInstruments[];
extern int GlobalInstrumentsNum;

// ������
extern int iRequestID;

extern CTradeInfo* gTradeInfo;

void CMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast)
{
	cerr << "--->>> "<< "OnRspError" << endl;
	IsErrorRspInfo(pRspInfo);
}

void CMdSpi::OnFrontDisconnected(int nReason)
{
	cerr << "--->>> " << "OnFrontDisconnected" << endl;
	cerr << "--->>> Reason = " << nReason << endl;
	gTradeInfo->setStatus(StatusDisconnect);
}
		
void CMdSpi::OnHeartBeatWarning(int nTimeLapse)
{
	cerr << "--->>> " << "OnHeartBeatWarning" << endl;
	cerr << "--->>> nTimerLapse = " << nTimeLapse << endl;
}

void CMdSpi::OnFrontConnected()
{
	cerr << "--->>> " << "OnFrontConnected" << endl;
	///�û���¼����
	ReqUserLogin();
}

void CMdSpi::ReqUserLogin()
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, BROKER_ID);
	strcpy(req.UserID, INVESTOR_ID);
	strcpy(req.Password, PASSWORD);
	int iResult = pMarketApi->ReqUserLogin(&req, ++iRequestID);
	cerr << "--->>> �����û���¼����: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;
}

void CMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspUserLogin" << endl;
	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		///��ȡ��ǰ������
		cerr << "--->>> ��ȡ��ǰ������ = " << pMarketApi->GetTradingDay() << endl;
		// ����������
		SubscribeMarketData();	
		// ������ѯ��,ֻ�ܶ���֣������ѯ�ۣ�����������ͨ��traderapi��Ӧ�ӿڷ���
		SubscribeForQuoteRsp();	
	}
}

void CMdSpi::SubscribeMarketData()
{
	int iResult = pMarketApi->SubscribeMarketData(GlobalInstruments, GlobalInstrumentsNum);
	cerr << "--->>> �������鶩������: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;
}

void CMdSpi::SubscribeForQuoteRsp()
{
	int iResult = pMarketApi->SubscribeForQuoteRsp(GlobalInstruments, GlobalInstrumentsNum);
	cerr << "--->>> ����ѯ�۶�������: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;
}

void CMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "OnRspSubMarketData" << endl;
}

void CMdSpi::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "OnRspSubForQuoteRsp" << endl;
}

void CMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "OnRspUnSubMarketData" << endl;
}

void CMdSpi::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "OnRspUnSubForQuoteRsp" << endl;
}

void CMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	//cerr << "OnRtnDepthMarketData" << endl;
	DepthValue value;
	memset(&value, 0, sizeof(DepthValue));
	memcpy(value.name, pDepthMarketData->InstrumentID, strlen(pDepthMarketData->InstrumentID));
	value.askPrice = pDepthMarketData->AskPrice1;
	value.askVolumn = pDepthMarketData->AskVolume1;
	value.bidPrice = pDepthMarketData->BidPrice1;
	value.bidVolumn = pDepthMarketData->BidVolume1;
	gTradeInfo->updateDepth(value);
}

void CMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{
	cerr << "OnRtnForQuoteRsp" << endl;
}

bool CMdSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	// ���ErrorID != 0, ˵���յ��˴������Ӧ
	bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bResult)
		cerr << "--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << endl;
	return bResult;
}