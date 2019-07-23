#include <windows.h>
#include <iostream>
using namespace std;

#include ".\ThostTraderApi\ThostFtdcTraderApi.h"
#include "TraderSpi.h"
#include "TradeInfo.h"

#pragma warning(disable : 4996)

// USER_API����
extern CThostFtdcTraderApi* pTraderApi;

// ���ò���
extern char BROKER_ID[];		// ���͹�˾����
extern char INVESTOR_ID[];		// Ͷ���ߴ���
extern char PASSWORD[];			// �û�����
//extern char INSTRUMENT_ID[];	// ��Լ����
extern char *GlobalInstruments[];
extern int GlobalInstrumentsNum;
extern char GlobalAppID[];
extern char GlobalAuthenCode[];

// ������
extern int iRequestID;
extern CTradeInfo* gTradeInfo;

// �Ự����
TThostFtdcFrontIDType	FRONT_ID;	//ǰ�ñ��
TThostFtdcSessionIDType	SESSION_ID;	//�Ự���
TThostFtdcOrderRefType	ORDER_REF;	//��������
TThostFtdcOrderRefType	EXECORDER_REF;	//ִ����������
TThostFtdcOrderRefType	FORQUOTE_REF;	//ѯ������
TThostFtdcOrderRefType	QUOTE_REF;	//��������

// �����ж�
bool IsFlowControl(int iResult)
{
	return ((iResult == -2) || (iResult == -3));
}

void CTraderSpi::OnFrontConnected()
{
	cerr << "--->>> " << "OnFrontConnected" << endl;
	static const char *version = pTraderApi->GetApiVersion();
	cout << "------��ǰ�汾�� ��" << version << " ------" << endl;
	ReqAuthenticate();
}

int CTraderSpi::ReqAuthenticate()
{
	CThostFtdcReqAuthenticateField field;
	memset(&field, 0, sizeof(field));
	strcpy(field.BrokerID, BROKER_ID);
	strcpy(field.UserID, INVESTOR_ID);
	strcpy(field.AppID, GlobalAppID);
	strcpy(field.AuthCode, GlobalAuthenCode);
	return pTraderApi->ReqAuthenticate(&field, ++iRequestID);
}

void CTraderSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	printf("<OnRspAuthenticate>\n");
	if (pRspAuthenticateField)
	{
		printf("\tBrokerID [%s]\n", pRspAuthenticateField->BrokerID);
		printf("\tUserID [%s]\n", pRspAuthenticateField->UserID);
		printf("\tUserProductInfo [%s]\n", pRspAuthenticateField->UserProductInfo);
		printf("\tAppID [%s]\n", pRspAuthenticateField->AppID);
		printf("\tAppType [%c]\n", pRspAuthenticateField->AppType);
	}
	if (pRspInfo)
	{
		printf("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
		printf("\tErrorID [%d]\n", pRspInfo->ErrorID);
	}
	printf("\tnRequestID [%d]\n", nRequestID);
	printf("\tbIsLast [%d]\n", bIsLast);
	printf("</OnRspAuthenticate>\n");

	///�û���¼����
	ReqUserLogin();
};

void CTraderSpi::ReqUserLogin()
{
	mIndex = 0;
	if (!gTradeInfo->setStatus(StatusProcess)) {
		return;
	}
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, BROKER_ID);
	strcpy(req.UserID, INVESTOR_ID);
	strcpy(req.Password, PASSWORD);
	int iResult = pTraderApi->ReqUserLogin(&req, ++iRequestID);
	cerr << "--->>> �����û���¼����: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
}

void CTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspUserLogin" << endl;
	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		// ����Ự����
		FRONT_ID = pRspUserLogin->FrontID;
		SESSION_ID = pRspUserLogin->SessionID;
		int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
		iNextOrderRef++;
		sprintf(ORDER_REF, "%d", iNextOrderRef);
		sprintf(EXECORDER_REF, "%d", 1);
		sprintf(FORQUOTE_REF, "%d", 1);
		sprintf(QUOTE_REF, "%d", 1);
		///��ȡ��ǰ������
		cerr << "--->>> ��ȡ��ǰ������ = " << pTraderApi->GetTradingDay() << endl;
		cerr << "--->>> ��ǰSessioID = " << hex << pRspUserLogin->SessionID << endl;
		///Ͷ���߽�����ȷ��
		ReqSettlementInfoConfirm();
	}
}

void CTraderSpi::ReqSettlementInfoConfirm()
{
	CThostFtdcSettlementInfoConfirmField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, BROKER_ID);
	strcpy(req.InvestorID, INVESTOR_ID);
	int iResult = pTraderApi->ReqSettlementInfoConfirm(&req, ++iRequestID);
	cerr << "--->>> Ͷ���߽�����ȷ��: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
}

void CTraderSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspSettlementInfoConfirm" << endl;
	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		// ��ȡ�˻�����
		//ReqQryTradingAccount();
		gTradeInfo->setStatus(StatusDone);
	}
}

void CTraderSpi::ReqQryInstrument(char *instrumentID)
{
	if (!gTradeInfo->setStatus(StatusProcess)) {
		return;
	}

	CThostFtdcQryInstrumentField req;
	memset(&req, 0, sizeof(req));
	memcpy(req.InstrumentID, instrumentID, strlen(instrumentID));
	while (true)
	{
		int iResult = pTraderApi->ReqQryInstrument(&req, ++iRequestID);
		if (!IsFlowControl(iResult))
		{
			cerr << "--->>> �����ѯ��Լ: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
			break;
		}
		else
		{
			cerr << "--->>> �����ѯ��Լ: " << iResult << ", �ܵ�����" << endl;
			Sleep(1000);
		}
	} // while


}

void CTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	// ��Щ��Ʒ�᷵�ض�����Ϣ
	cerr << "--->>> " << "OnRspQryInstrument:" << pInstrument->InstrumentID << " " << bIsLast << endl;
	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		gTradeInfo->saveInstrumentInfo(pInstrument);
		gTradeInfo->setStatus(StatusDone);
	}
	else if (!IsErrorRspInfo(pRspInfo)) {
		gTradeInfo->saveInstrumentInfo(pInstrument);
	}
}

void CTraderSpi::ReqQryTradingAccount()
{

	if (!gTradeInfo->setStatus(StatusProcess)) {
		return;
	}

	CThostFtdcQryTradingAccountField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, BROKER_ID);
	strcpy(req.InvestorID, INVESTOR_ID);
	while (true)
	{
		int iResult = pTraderApi->ReqQryTradingAccount(&req, ++iRequestID);
		if (!IsFlowControl(iResult))
		{
			cerr << "--->>> �����ѯ�ʽ��˻�: "  << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
			break;
		}
		else
		{
			cerr << "--->>> �����ѯ�ʽ��˻�: "  << iResult << ", �ܵ�����" << endl;
			Sleep(1000);
		}
	} // while
}

void CTraderSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspQryTradingAccount" << endl;
	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		gTradeInfo->saveAccountInfo(pTradingAccount);
		gTradeInfo->setStatus(StatusDone);

	}
}

void CTraderSpi::ReqQryInvestorPosition(char *instrumentID)
{
	if (!gTradeInfo->setStatus(StatusProcess)) {
		return;
	}

	CThostFtdcQryInvestorPositionField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.BrokerID, BROKER_ID);
	strcpy(req.InvestorID, INVESTOR_ID);
	strcpy(req.InstrumentID, instrumentID);

	while (true)
	{
		int iResult = pTraderApi->ReqQryInvestorPosition(&req, ++iRequestID);
		if (!IsFlowControl(iResult))
		{
			cerr << "--->>> �����ѯͶ���ֲ߳�: "  << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
			break;
		}
		else
		{
			cerr << "--->>> �����ѯͶ���ֲ߳�: "  << iResult << ", �ܵ�����" << endl;
			Sleep(1000);
		}
	} // while
}

void CTraderSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	// �Ϻ��ڻ���������̨�᷵���������ݣ���һ�������ղ�λ���ڶ�����ղ�λ��������δ֪
	cerr << "--->>> " << "OnRspQryInvestorPosition" << " Last:" << bIsLast << endl;
	//if (bIsLast && !IsErrorRspInfo(pRspInfo))
	if(!IsErrorRspInfo(pRspInfo) && pInvestorPosition != NULL)
	{
		///����¼������
		//ReqOrderInsert();
		////ִ������¼������
		//ReqExecOrderInsert();
		////ѯ��¼��
		//ReqForQuoteInsert();
		////�����̱���¼��
		//ReqQuoteInsert();
		if (pInvestorPosition->Position > 0) {
			gTradeInfo->savePositionInfo(pInvestorPosition);
		}
	}

	if (bIsLast) {
		gTradeInfo->setStatus(StatusDone);
	}
}

void CTraderSpi::ReqMarketOpenInsert(char *instrumentID, int volume, int limitPrice, bool isBuy, bool isMarket)
{
	if (!gTradeInfo->setStatus(StatusProcess)) {
		return;
	}

	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, BROKER_ID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, INVESTOR_ID);
	///��Լ����
	strcpy(req.InstrumentID, instrumentID);
	///��������
	strcpy(req.OrderRef, "");
	///�û�����
	//	TThostFtdcUserIDType	UserID;
	///��������: 
	if (isBuy) {
		req.Direction = THOST_FTDC_D_Buy;
	}
	else {
		req.Direction = THOST_FTDC_D_Sell;
	}
	///��Ͽ�ƽ��־: ����
	req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	///���Ͷ���ױ���־
	req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;

	if (isMarket) {
		///�����۸�����: �޼�
		req.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
		///�۸�
		req.LimitPrice = 0;
		/////��Ч������: �����ɽ�
		//req.TimeCondition = THOST_FTDC_TC_IOC;
	}
	else {
		///�����۸�����: �޼�
		req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		///�۸�
		req.LimitPrice = limitPrice;
		/////��Ч������: ������Ч
		//req.TimeCondition = THOST_FTDC_TC_GFD;
	}

	///��Ч������: �����ɽ�,�Ҳ���Ҫ��������[mqiu20190612]
	req.TimeCondition = THOST_FTDC_TC_IOC;

	///����: 1
	req.VolumeTotalOriginal = volume;

	///GTD����
	//	TThostFtdcDateType	GTDDate;
	///�ɽ�������: �κ�����
	req.VolumeCondition = THOST_FTDC_VC_MV;
	///��С�ɽ���: 1
	req.MinVolume = volume;
	///��������: ����
	req.ContingentCondition = THOST_FTDC_CC_Immediately;
	///ֹ���
	//	TThostFtdcPriceType	StopPrice;
	///ǿƽԭ��: ��ǿƽ
	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///�Զ������־: ��
	req.IsAutoSuspend = 0;
	///ҵ��Ԫ
	//	TThostFtdcBusinessUnitType	BusinessUnit;
	///������
	//	TThostFtdcRequestIDType	RequestID;
	///�û�ǿ����־: ��
	req.UserForceClose = 0;

	int iResult = pTraderApi->ReqOrderInsert(&req, ++iRequestID);
	cerr << "--->>> �мۿ���¼������: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
}

void CTraderSpi::ReqMarketCloseInsert(char *instrumentID, int volume, int limitPrice, bool isBuy, bool isMarket, bool isToday)
{

	if (!gTradeInfo->setStatus(StatusProcess)) {
		return;
	}


	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, BROKER_ID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, INVESTOR_ID);
	///��Լ����
	strcpy(req.InstrumentID, instrumentID);
	///��������
	strcpy(req.OrderRef, "");
	///�û�����
	//	TThostFtdcUserIDType	UserID;
	///��������: 
	if (isBuy) {
		req.Direction = THOST_FTDC_D_Buy;
	}
	else {
		req.Direction = THOST_FTDC_D_Sell;
	}

	if (isToday){
		///��Ͽ�ƽ��־: ƽ��
		req.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
	}
	else {
		///��Ͽ�ƽ��־: ƽ��
		req.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
	}

	///���Ͷ���ױ���־
	req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;

	if (isMarket) {
		///�����۸�����: �޼�
		req.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
		///�۸�
		req.LimitPrice = 0;
	}
	else {
		///�����۸�����: �޼�
		req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		///�۸�
		req.LimitPrice = limitPrice;
	}

	///����: 1
	req.VolumeTotalOriginal = volume;
	///��Ч������: �����ɽ�
	req.TimeCondition = THOST_FTDC_TC_IOC;
	///GTD����
	//	TThostFtdcDateType	GTDDate;
	///�ɽ�������: �κ�����
	req.VolumeCondition = THOST_FTDC_VC_MV;
	///��С�ɽ���: 1
	req.MinVolume = volume;
	///��������: ����
	req.ContingentCondition = THOST_FTDC_CC_Immediately;
	///ֹ���
	//	TThostFtdcPriceType	StopPrice;
	///ǿƽԭ��: ��ǿƽ
	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///�Զ������־: ��
	req.IsAutoSuspend = 0;
	///ҵ��Ԫ
	//	TThostFtdcBusinessUnitType	BusinessUnit;
	///������
	//	TThostFtdcRequestIDType	RequestID;
	///�û�ǿ����־: ��
	req.UserForceClose = 0;

	int iResult = pTraderApi->ReqOrderInsert(&req, ++iRequestID);
	cerr << "--->>> �м�ƽ��¼������: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
}

void CTraderSpi::ReqMarketStopPriceInsert(char *instrumentID, int volume, bool isBuy, double stopPrice, double limitPrice)
{

	if (!gTradeInfo->setStatus(StatusProcess)) {
		return;
	}


	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, BROKER_ID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, INVESTOR_ID);
	///��Լ����
	strcpy(req.InstrumentID, instrumentID);
	///��������
	strcpy(req.OrderRef, "");
	///�û�����
	//	TThostFtdcUserIDType	UserID;
	///�����۸�����: �޼�
	req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	///��������: 
	if (isBuy) {
		req.Direction = THOST_FTDC_D_Buy;
		///��������: ����
		req.ContingentCondition = THOST_FTDC_CC_LastPriceLesserThanStopPrice;
	}
	else {
		req.Direction = THOST_FTDC_D_Sell;
		///��������: ����
		req.ContingentCondition = THOST_FTDC_CC_LastPriceGreaterThanStopPrice;
	}
	///ֹ���
	req.StopPrice = stopPrice;
	///��Ͽ�ƽ��־: ƽ��
	req.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
	///���Ͷ���ױ���־
	req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///�۸�
	req.LimitPrice = limitPrice;
	///����: 1
	req.VolumeTotalOriginal = volume;
	///��Ч������: ������Ч
	req.TimeCondition = THOST_FTDC_TC_GFD;
	///GTD����
	//	TThostFtdcDateType	GTDDate;
	///�ɽ�������: �κ�����
	req.VolumeCondition = THOST_FTDC_VC_MV;
	///��С�ɽ���: 1
	req.MinVolume = volume;
	///ǿƽԭ��: ��ǿƽ
	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///�Զ������־: ��
	req.IsAutoSuspend = 0;
	///ҵ��Ԫ
	//	TThostFtdcBusinessUnitType	BusinessUnit;
	///������
	//	TThostFtdcRequestIDType	RequestID;
	///�û�ǿ����־: ��
	req.UserForceClose = 0;

	int iResult = pTraderApi->ReqOrderInsert(&req, ++iRequestID);
	cerr << "--->>> ֹ��¼������: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
}


void CTraderSpi::ReqOrderInsert(char *instrumentID, int volume, bool isBuy, double price)
{
	if (!gTradeInfo->setStatus(StatusProcess)) {
		return;
	}

	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, BROKER_ID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, INVESTOR_ID);
	///��Լ����
	//strcpy(req.InstrumentID, INSTRUMENT_ID);
	///��������
	strcpy(req.OrderRef, ORDER_REF);
	///�û�����
//	TThostFtdcUserIDType	UserID;
	///�����۸�����: �޼�
	req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	///��������: 
	if (isBuy) {
		req.Direction = THOST_FTDC_D_Buy;
	}
	else {
		req.Direction = THOST_FTDC_D_Sell;
	}
	///��Ͽ�ƽ��־: ����
	req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	///���Ͷ���ױ���־
	req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///�۸�
	req.LimitPrice = price;
	///����: 1
	req.VolumeTotalOriginal = volume;
	///��Ч������: ������Ч
	req.TimeCondition = THOST_FTDC_TC_GFD;
	///GTD����
//	TThostFtdcDateType	GTDDate;
	///�ɽ�������: �κ�����
	req.VolumeCondition = THOST_FTDC_VC_MV;
	///��С�ɽ���: 1
	req.MinVolume = volume;
	///��������: ����
	req.ContingentCondition = THOST_FTDC_CC_Immediately;
	///ֹ���
//	TThostFtdcPriceType	StopPrice;
	///ǿƽԭ��: ��ǿƽ
	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///�Զ������־: ��
	req.IsAutoSuspend = 0;
	///ҵ��Ԫ
//	TThostFtdcBusinessUnitType	BusinessUnit;
	///������
//	TThostFtdcRequestIDType	RequestID;
	///�û�ǿ����־: ��
	req.UserForceClose = 0;

	int iResult = pTraderApi->ReqOrderInsert(&req, ++iRequestID);
	cerr << "--->>> ����¼������: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
}

//ִ������¼������
void CTraderSpi::ReqExecOrderInsert()
{
	CThostFtdcInputExecOrderField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, BROKER_ID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, INVESTOR_ID);
	///��Լ����
	//strcpy(req.InstrumentID, INSTRUMENT_ID);
	///��������
	strcpy(req.ExecOrderRef, EXECORDER_REF);
	///�û�����
	//	TThostFtdcUserIDType	UserID;
	///����
	req.Volume=1;
	///������
	//TThostFtdcRequestIDType	RequestID;
	///ҵ��Ԫ
	//TThostFtdcBusinessUnitType	BusinessUnit;
	///��ƽ��־
	req.OffsetFlag=THOST_FTDC_OF_Close;//���������������Ҫ��ƽ���ƽ��
	///Ͷ���ױ���־
	req.HedgeFlag=THOST_FTDC_HF_Speculation;
	///ִ������
	req.ActionType=THOST_FTDC_ACTP_Exec;//�������ִ������THOST_FTDC_ACTP_Abandon
	///����ͷ������ĳֲַ���
	req.PosiDirection=THOST_FTDC_PD_Long;
	///��Ȩ��Ȩ���Ƿ����ڻ�ͷ��ı��
	req.ReservePositionFlag=THOST_FTDC_EOPF_UnReserve;//�����н��������������֣������THOST_FTDC_EOPF_Reserve
	///��Ȩ��Ȩ�����ɵ�ͷ���Ƿ��Զ�ƽ��
	req.CloseFlag=THOST_FTDC_EOCF_AutoClose;//�����н��������������֣������THOST_FTDC_EOCF_NotToClose

	int iResult = pTraderApi->ReqExecOrderInsert(&req, ++iRequestID);
	cerr << "--->>> ִ������¼������: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
}

//ѯ��¼������
void CTraderSpi::ReqForQuoteInsert()
{
	CThostFtdcInputForQuoteField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, BROKER_ID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, INVESTOR_ID);
	///��Լ����
	//strcpy(req.InstrumentID, INSTRUMENT_ID);
	///��������
	strcpy(req.ForQuoteRef, EXECORDER_REF);
	///�û�����
	//	TThostFtdcUserIDType	UserID;

	int iResult = pTraderApi->ReqForQuoteInsert(&req, ++iRequestID);
	cerr << "--->>> ѯ��¼������: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
}
//����¼������
void CTraderSpi::ReqQuoteInsert()
{
	CThostFtdcInputQuoteField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, BROKER_ID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, INVESTOR_ID);
	///��Լ����
	//strcpy(req.InstrumentID, INSTRUMENT_ID);
	///��������
	strcpy(req.QuoteRef, QUOTE_REF);
	///���۸�
	//req.AskPrice=LIMIT_PRICE; // by mqiu20180526
	///��۸�
	//req.BidPrice=LIMIT_PRICE-1.0; // by mqiu20180526
	///������
	req.AskVolume=1;
	///������
	req.BidVolume=1;
	///������
	//TThostFtdcRequestIDType	RequestID;
	///ҵ��Ԫ
	//TThostFtdcBusinessUnitType	BusinessUnit;
	///����ƽ��־
	req.AskOffsetFlag=THOST_FTDC_OF_Open;
	///��ƽ��־
	req.BidOffsetFlag=THOST_FTDC_OF_Open;
	///��Ͷ���ױ���־
	req.AskHedgeFlag=THOST_FTDC_HF_Speculation;
	///��Ͷ���ױ���־
	req.BidHedgeFlag=THOST_FTDC_HF_Speculation;
	
	int iResult = pTraderApi->ReqQuoteInsert(&req, ++iRequestID);
	cerr << "--->>> ����¼������: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
}

void CTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspOrderInsert" << endl;
	IsErrorRspInfo(pRspInfo);
	gTradeInfo->updateTradeResult(TradeError, NULL, pRspInfo->ErrorMsg);
	gTradeInfo->setStatus(StatusDone);
}

void CTraderSpi::OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//���ִ��������ȷ���򲻻����ûص�
	cerr << "--->>> " << "OnRspExecOrderInsert" << endl;
	IsErrorRspInfo(pRspInfo);
}

void CTraderSpi::OnRspForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//���ѯ����ȷ���򲻻����ûص�
	cerr << "--->>> " << "OnRspForQuoteInsert" << endl;
	IsErrorRspInfo(pRspInfo);
}

void CTraderSpi::OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//���������ȷ���򲻻����ûص�
	cerr << "--->>> " << "OnRspQuoteInsert" << endl;
	IsErrorRspInfo(pRspInfo);
}


void CTraderSpi::ReqCancelOrder(char *instrumentID, char *exchangeID, char *orderSysID)
{
	if (!gTradeInfo->setStatus(StatusProcess)) {
		cerr << "Invalid status" << endl;
		return;
	}

	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, BROKER_ID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, INVESTOR_ID);
	///������������
	//	TThostFtdcOrderActionRefType	OrderActionRef;
	///��������
	strcpy(req.ExchangeID, exchangeID);
	strcpy(req.OrderSysID, orderSysID);
	///������
	//	TThostFtdcRequestIDType	RequestID;
	///ǰ�ñ��
	req.FrontID = FRONT_ID;
	///�Ự���
	req.SessionID = SESSION_ID;
	///����������
	//	TThostFtdcExchangeIDType	ExchangeID;
	///�������
	//	TThostFtdcOrderSysIDType	OrderSysID;
	///������־
	req.ActionFlag = THOST_FTDC_AF_Delete;
	///�۸�
	//	TThostFtdcPriceType	LimitPrice;
	///�����仯
	//	TThostFtdcVolumeType	VolumeChange;
	///�û�����
	//	TThostFtdcUserIDType	UserID;
	///��Լ����
	strcpy(req.InstrumentID, instrumentID);

	int iResult = pTraderApi->ReqOrderAction(&req, ++iRequestID);
	cerr << "--->>> ������������: " << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
}

void CTraderSpi::ReqOrderAction(CThostFtdcOrderField *pOrder)
{
	static bool ORDER_ACTION_SENT = false;		//�Ƿ����˱���
	if (ORDER_ACTION_SENT)
		return;

	CThostFtdcInputOrderActionField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, pOrder->BrokerID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, pOrder->InvestorID);
	///������������
//	TThostFtdcOrderActionRefType	OrderActionRef;
	///��������
	strcpy(req.OrderRef, pOrder->OrderRef);
	///������
//	TThostFtdcRequestIDType	RequestID;
	///ǰ�ñ��
	req.FrontID = FRONT_ID;
	///�Ự���
	req.SessionID = SESSION_ID;
	///����������
//	TThostFtdcExchangeIDType	ExchangeID;
	///�������
//	TThostFtdcOrderSysIDType	OrderSysID;
	///������־
	req.ActionFlag = THOST_FTDC_AF_Delete;
	///�۸�
//	TThostFtdcPriceType	LimitPrice;
	///�����仯
//	TThostFtdcVolumeType	VolumeChange;
	///�û�����
//	TThostFtdcUserIDType	UserID;
	///��Լ����
	strcpy(req.InstrumentID, pOrder->InstrumentID);

	int iResult = pTraderApi->ReqOrderAction(&req, ++iRequestID);
	cerr << "--->>> ������������: "  << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
	ORDER_ACTION_SENT = true;
}

void CTraderSpi::ReqExecOrderAction(CThostFtdcExecOrderField *pExecOrder)
{
	static bool EXECORDER_ACTION_SENT = false;		//�Ƿ����˱���
	if (EXECORDER_ACTION_SENT)
		return;

	CThostFtdcInputExecOrderActionField req;
	memset(&req, 0, sizeof(req));

	///���͹�˾����
	strcpy(req.BrokerID,pExecOrder->BrokerID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID,pExecOrder->InvestorID);
	///ִ�������������
	//TThostFtdcOrderActionRefType	ExecOrderActionRef;
	///ִ����������
	strcpy(req.ExecOrderRef,pExecOrder->ExecOrderRef);
	///������
	//TThostFtdcRequestIDType	RequestID;
	///ǰ�ñ��
	req.FrontID=FRONT_ID;
	///�Ự���
	req.SessionID=SESSION_ID;
	///����������
	//TThostFtdcExchangeIDType	ExchangeID;
	///ִ������������
	//TThostFtdcExecOrderSysIDType	ExecOrderSysID;
	///������־
	req.ActionFlag=THOST_FTDC_AF_Delete;
	///�û�����
	//TThostFtdcUserIDType	UserID;
	///��Լ����
	strcpy(req.InstrumentID,pExecOrder->InstrumentID);

	int iResult = pTraderApi->ReqExecOrderAction(&req, ++iRequestID);
	cerr << "--->>> ִ�������������: "  << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
	EXECORDER_ACTION_SENT = true;
}

void CTraderSpi::ReqQuoteAction(CThostFtdcQuoteField *pQuote)
{
	static bool QUOTE_ACTION_SENT = false;		//�Ƿ����˱���
	if (QUOTE_ACTION_SENT)
		return;

	CThostFtdcInputQuoteActionField req;
	memset(&req, 0, sizeof(req));
	///���͹�˾����
	strcpy(req.BrokerID, pQuote->BrokerID);
	///Ͷ���ߴ���
	strcpy(req.InvestorID, pQuote->InvestorID);
	///���۲�������
	//TThostFtdcOrderActionRefType	QuoteActionRef;
	///��������
	strcpy(req.QuoteRef,pQuote->QuoteRef);
	///������
	//TThostFtdcRequestIDType	RequestID;
	///ǰ�ñ��
	req.FrontID=FRONT_ID;
	///�Ự���
	req.SessionID=SESSION_ID;
	///����������
	//TThostFtdcExchangeIDType	ExchangeID;
	///���۲������
	//TThostFtdcOrderSysIDType	QuoteSysID;
	///������־
	req.ActionFlag=THOST_FTDC_AF_Delete;
	///�û�����
	//TThostFtdcUserIDType	UserID;
	///��Լ����
	strcpy(req.InstrumentID,pQuote->InstrumentID);

	int iResult = pTraderApi->ReqQuoteAction(&req, ++iRequestID);
	cerr << "--->>> ���۲�������: "  << iResult << ((iResult == 0) ? ", �ɹ�" : ", ʧ��") << endl;
	QUOTE_ACTION_SENT = true;
}

void CTraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspOrderAction" << endl;
	IsErrorRspInfo(pRspInfo);
}

void CTraderSpi::OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInpuExectOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//��ȷ�ĳ����������������ûص�
	cerr << "--->>> " << "OnRspExecOrderAction" << endl;
	IsErrorRspInfo(pRspInfo);
}

void CTraderSpi::OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInpuQuoteAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	//��ȷ�ĳ����������������ûص�
	cerr << "--->>> " << "OnRspQuoteAction" << endl;
	IsErrorRspInfo(pRspInfo);
}

///����֪ͨ
void CTraderSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	cerr << "--->>> " << "OnRtnOrder"  << endl;
	if (IsMyOrder(pOrder))
	{
		//if (IsTradingOrder(pOrder))
		//	ReqOrderAction(pOrder);
		//else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
		//	cout << "--->>> �����ɹ�" << endl;
		cout << "����״̬:" << pOrder->OrderStatus <<"���ױ��:"<< pOrder->OrderSysID << endl;
		// ֻ��OnRtnTrade�ص�����ʱ���óɽ�
		if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled) {
			cout << "���׳���:" << pOrder->OrderStatus << endl;
			gTradeInfo->updateTradeResult(TradeCancled, NULL, NULL);
			gTradeInfo->setStatus(StatusDone);
		}
		else {
			cout << "��������:" << pOrder->TimeCondition << endl;
			// Ŀǰδʹ��THOST_FTDC_TC_GFD��ǿ�����ж�������ִ��
			if (pOrder->TimeCondition == THOST_FTDC_TC_GFD) {	// �޼۵�����Ŀǰ�Ƕ��ڶ����У�����״̬�����TODO
				if (pOrder->OrderStatus == THOST_FTDC_OST_NoTradeQueueing) {
					gTradeInfo->updateQueueInfo(TradeQueued,pOrder->ExchangeID, pOrder->OrderSysID);
					gTradeInfo->setStatus(StatusDone);
				}
			}
			else if (pOrder->TimeCondition == THOST_FTDC_TC_IOC) {	// �м۵���Ҫȫ���ɽ�
				if (pOrder->OrderStatus == THOST_FTDC_OST_AllTraded) {
					gTradeInfo->setStatus(StatusAllTraded);
					gTradeInfo->setOrderSysID(pOrder->OrderSysID);
				}
			}
		}
	}
	else {
		cerr << "����ͬһ���Ự, ���ױ��:"<< pOrder->OrderSysID << endl;
	}
}

//ִ������֪ͨ
void CTraderSpi::OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder)
{
	cerr << "--->>> " << "OnRtnExecOrder"  << endl;
	if (IsMyExecOrder(pExecOrder))
	{
		if (IsTradingExecOrder(pExecOrder))
			ReqExecOrderAction(pExecOrder);
		else if (pExecOrder->ExecResult == THOST_FTDC_OER_Canceled)
			cout << "--->>> ִ�����泷���ɹ�" << endl;
	}
}

//ѯ��֪ͨ
void CTraderSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{
	//�������н���ѯ��֪ͨͨ���ýӿڷ��أ�ֻ�������̿ͻ������յ���֪ͨ
	cerr << "--->>> " << "OnRtnForQuoteRsp"  << endl;
}

//����֪ͨ
void CTraderSpi::OnRtnQuote(CThostFtdcQuoteField *pQuote)
{
	cerr << "--->>> " << "OnRtnQuote"  << endl;
	if (IsMyQuote(pQuote))
	{
		if (IsTradingQuote(pQuote))
			ReqQuoteAction(pQuote);
		else if (pQuote->QuoteStatus == THOST_FTDC_OST_Canceled)
			cout << "--->>> ���۳����ɹ�" << endl;
	}
}

///�ɽ�֪ͨ
void CTraderSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	cerr << "--->>> " << "OnRtnTrade"  << endl;
	if (gTradeInfo->isCurrentOrderSysID(pTrade->OrderSysID)) {
		if (gTradeInfo->getStatus() == StatusAllTraded) {
			cout << "��ȫ�ɽ��۸�:" << pTrade->Price << "�ɽ�����:" << pTrade->Volume << endl;
			gTradeInfo->updateTradeInfo(pTrade->Price, pTrade->Volume);
			gTradeInfo->updateTradeResult(TradeDone, pTrade, NULL);
			gTradeInfo->setStatus(StatusDone);
		}
		else {
			cout << "���ֳɽ��۸�:" << pTrade->Price << "�ɽ�����:" << pTrade->Volume << endl;
			gTradeInfo->updateTradeInfo(pTrade->Price, pTrade->Volume);
		}
	}
	else {
		cout<<"�Ǳ��Ự���ױ���:"<< pTrade->OrderSysID << "�ɽ��۸�:" << pTrade->Price << "�ɽ�����:" << pTrade->Volume << endl;
	}


}

void CTraderSpi:: OnFrontDisconnected(int nReason)
{
	cerr << "--->>> " << "OnFrontDisconnected" << endl;
	cerr << "--->>> Reason = " << nReason << endl;
	gTradeInfo->setStatus(StatusDisconnect);
}
		
void CTraderSpi::OnHeartBeatWarning(int nTimeLapse)
{
	cerr << "--->>> " << "OnHeartBeatWarning" << endl;
	cerr << "--->>> nTimerLapse = " << nTimeLapse << endl;
}

void CTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	cerr << "--->>> " << "OnRspError" << endl;
	IsErrorRspInfo(pRspInfo);
}

bool CTraderSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	// ���ErrorID != 0, ˵���յ��˴������Ӧ
	bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bResult) {
		cerr << "--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << endl;
		gTradeInfo->setStatus(StatusError);
	}

	return bResult;
}

bool CTraderSpi::IsMyOrder(CThostFtdcOrderField *pOrder)
{
	//return ((pOrder->FrontID == FRONT_ID) &&
	//		(pOrder->SessionID == SESSION_ID) &&
	//		(strcmp(pOrder->OrderRef, ORDER_REF) == 0));

	cout << "Ordrer Front:" << pOrder->FrontID << "Local Front:" << FRONT_ID << endl;
	cout << "Ordrer SessionID:" << pOrder->SessionID << "Local SessionID:" << SESSION_ID << endl;

	return ((pOrder->FrontID == FRONT_ID) && (pOrder->SessionID == SESSION_ID));

	//return true;
}

bool CTraderSpi::IsMyExecOrder(CThostFtdcExecOrderField *pExecOrder)
{
	return ((pExecOrder->FrontID == FRONT_ID) &&
		(pExecOrder->SessionID == SESSION_ID) &&
		(strcmp(pExecOrder->ExecOrderRef, EXECORDER_REF) == 0));
}

bool CTraderSpi::IsMyQuote(CThostFtdcQuoteField *pQuote)
{
	return ((pQuote->FrontID == FRONT_ID) &&
		(pQuote->SessionID == SESSION_ID) &&
		(strcmp(pQuote->QuoteRef, QUOTE_REF) == 0));
}

bool CTraderSpi::IsTradingOrder(CThostFtdcOrderField *pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
			(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
			(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

bool CTraderSpi::IsTradingExecOrder(CThostFtdcExecOrderField *pExecOrder)
{
	return (pExecOrder->ExecResult != THOST_FTDC_OER_Canceled);
}

bool CTraderSpi::IsTradingQuote(CThostFtdcQuoteField *pQuote)
{
	return (pQuote->QuoteStatus != THOST_FTDC_OST_Canceled);
}
