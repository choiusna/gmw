// party.h

#ifndef __PARTY_H__BY_SGCHOI
#define __PARTY_H__BY_SGCHOI   

#include "../util/config.h"
#include "../circuit/circuit.h"
#include "../util/altqueue.h"
#include "../util/socket.h"
#include "../util/thread.h"
#include "gmw.h"
#include <vector>
using namespace std;
 
class CParty
{
public:
	CParty(); 
	~CParty();  
	
	static CParty* GetInstance(CParty* p = NULL){
		static CParty* sp;
		if(p) sp = p; 
		return sp;
	}

public:
	BOOL Init();
	void Cleanup();
	void Run();	
	const vector<int>& GetOutput(){ return m_vOutputs; }
 
private:
	BOOL EstablishConnection();
	BOOL SetupOTExtension();
	BOOL ShareInput();
	BOOL ComputeMultiplications();
	BOOL ComputeOutput();

	 
private:
	// thread handlers
	BOOL ThreadRunConnect(int nID);  
	BOOL ThreadRunRecvInputShare(int nSndID);
	BOOL ThreadRunMultiplySender(int nRcvID);
	BOOL ThreadRunMultiplyReceiver(int nSndID);
	BOOL ThreadRunRecvOutputShare(int nSndID);

	BOOL MainThreadRunListen();
	BOOL MainThreadRunSendInputShare();
	BOOL MainThreadRunSendOutputShare();

	BOOL ThreadRunIKNPSndFirst(int nRcvID);
	BOOL ThreadRunIKNPSndSecond(int nRcvID);
	BOOL ThreadRunIKNPRcvFirst(int nSndID);
	BOOL ThreadRunIKNPRcvSecond(int nSndID);

	BOOL ThreadRunCircuit();

	enum EJobType
	{
		e_Connect,
		e_OTExt,
		e_ShareInput, 
		e_Mult,
		e_Output,
		e_Stop,
	};
	
	BOOL WakeupWorkerThreads(EJobType);
	BOOL WaitWorkerThreads();
	BOOL ThreadNotifyTaskDone(BOOL);
 
private:
	class CWorkerThread: public CThread
	{
	public:
		CWorkerThread(int i): m_nPartnerID(i){}
		void PutJob(EJobType e){ m_eJob = e; m_evt.Set(); }
		void ThreadMain();
		int			m_nPartnerID;
		CEvent		m_evt;
		EJobType	m_eJob;
	};

	class COTHelperThread: public CThread
	{
	public:
		COTHelperThread(int i): m_nPartnerID(i){}
		void ThreadMain();
		int			m_nPartnerID;
	};

	class CCircuitThread: public CThread
	{
	public:
		void ThreadMain(){ CParty::GetInstance()->ThreadRunCircuit(); }
	};


public:
	// Network Communication
	vector<CSocket>		m_vSockets;
	int					m_nNumParties;
	int					m_nPID;
	
	// Ciruit
	CCircuit*			m_pCircuit;
	GATE*				m_pGates;
	int					m_nANDGates;

	// Multiplication
	int					m_nMultIdx;
	CAltQueue			m_queOnGoings;
	vector<CBitVector>	m_vGMWRnd;

	// Output
	vector<CBitVector>	m_vOutputShares;
	CBitVector			m_Outputs;
	vector<int>			m_vOutputs;
		
	// OT Proprocessing randomness: use [WW06]
	vector<CBitVector>	m_vWWIn; 
	vector<CBitVector>	m_vWWOut; 
	
	// NTL: Naor-Pinkas OT
	ZZ					m_p;
	ZZ					m_q;
	ZZ					m_g;

	// IKNP:
	vector<CBitVector>	m_vIKNPT;
	vector<CBitVector>	m_vIKNPU;
	vector<CBitNPMatrix> m_vIKNPMtx;
	vector<int>			m_vIKNPProgress;

	// SHA PRG
	BYTE				m_aSeed[SHA1_BYTES];
	vector<int>			m_vCounter;

	// Thread Management
	vector<CWorkerThread*> m_vThreads;
	CEvent				m_evt;
	CLock				m_lock;
	int					m_nWorkingThreads;
	BOOL				m_bWorkerThreadSuccess;

#ifdef IMPL_PRIVIATE_CHANNEL
	vector<CBitVector>	m_vPKeys;
#endif //IMPL_PRIVIATE_CHANNEL	
};

#endif //__PARTY_H__BY_SGCHOI 

