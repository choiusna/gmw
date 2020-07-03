// server.h

#ifndef __SERVER_H__BY_SGCHOI
#define __SERVER_H__BY_SGCHOI

#include "../util/socket.h"
#include "../util/thread.h"
#include "yao.h"

class CServer
{
public:
	CServer(void){ GetInstance(this); }
	~CServer(void){}
	
	static CServer* GetInstance(CServer* p = NULL){
		static CServer* sp;
		if(p) sp = p; 
		return sp;
	}

public:
	BOOL Init();
	void Cleanup();
	void Run();		
	
	void RunWireThread();		// generate wires
	void RunGateThread();		// generate garbled circuit
	void RunMainThread();		// main thread runner: send garbled circuit
	void RunOTThread();			// ot protocols for client inputs

	void TestEvaluate();
	const vector<int>& GetOutput(){ return m_vOutput; }


public:
	class CWireThread: public CThread
	{
	public:
		void ThreadMain(){ CServer::GetInstance()->RunWireThread();}
	};

	class CGateThread: public CThread
	{
	public: 
		void ThreadMain(){ CServer::GetInstance()->RunGateThread();}
	};

	class COTThread: public CThread
	{
	public:
		void ThreadMain(){ CServer::GetInstance()->RunOTThread(); } 
	};
	 
public:
	CSocket			m_sockMain;
	CSocket			m_sockOT;
	BOOL			m_bStop;

	// circuit
	CCircuit*					m_pCircuit;
	GATE*						m_pGates;
	int							m_nNumGates;
 
	// yao
	YAO_WIRE*					m_pYaoWires;
	YAO_GARBLED_GATE*			m_pYaoGates;
	vector<int>					m_vOutGates;
	int							m_nGatesDone;
	int							m_nWiresDone;
	KEY*						m_pXORKey;


	// randomness
	BYTE						m_aSeed[20];
	int							m_nCounter;
	
	// batch parameters
	int							m_nKeyBatch;
	int							m_nGateBatch;


	// IKNP
	CBitVector					m_S;

	// output
	vector<int>					m_vOutput;
};


#endif //__SERVER_H__BY_SGCHOI

