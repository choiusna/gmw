//client.h
#ifndef __CLIENT_H__BY_SGCHOI
#define __CLIENT_H__BY_SGCHOI
 #include "../util/socket.h"
#include "../util/thread.h"
#include "yao.h"

  
class CClient
{
public:
	CClient(){ GetInstance(this); }
	~CClient(){}
	
	static CClient* GetInstance(CClient* p = NULL){
		static CClient* sp;
		if(p) sp = p; 
		return sp;
	}

public:
	void Init();
	void Cleanup();
	void Run();					

	void RunOTThread();			// ot protocols for client inputs
	void RunRecvThread();		// recv thread
	void RunMainThread();		// main thread runner: compute garbled circuit

	const vector<int>& GetOutput(){ return m_vOutput; }
	 

public:
	class COTThread: public CThread
	{
	public:
		void ThreadMain(){ CClient::GetInstance()->RunOTThread(); } 
	};
	 
	class CRecvThread: public CThread
	{
	public: 
		void ThreadMain(){ CClient::GetInstance()->RunRecvThread();}
	};

public:
	COTThread*		m_pThreadOT;
	CRecvThread*	m_pRecvThread;
	CSocket			m_sockMain;
	CSocket			m_sockOT;

	// random
	BYTE						m_aSeed[20];
	int							m_nCounter;


	// circuit
	class	CCircuit*			m_pCircuit;
	struct	GATE*				m_pGates;
	int							m_nNumGates;
	int							m_nGateStart;

	// yao
	struct  KEY*				m_pYaoKeys;
	struct	YAO_GARBLED_GATE*	m_pYaoGates;
	int							m_nGatesDone;
	BOOL						m_bOTDone;
	BOOL						m_bOutKeysReady;
	vector<KEY_PAIR>			m_vOutKeyPairs;
	vector<int>					m_vOutput;
 
		// batch
	int							m_nKeyBatch;
	int							m_nGateBatch;
	
	// IKNP
	vector<CBitVector>			m_T;
	CBitVector					m_r;
};

#endif //__CLIENT_H__BY_SGCHOI
