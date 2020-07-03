// altqueue.h
#ifndef __ALT_QUEUE_H__BY_SGCHOI
#define __ALT_QUEUE_H__BY_SGCHOI

#include "typedefs.h"

// Queue with Alternate operation: alternate queue buffer reading/writing.
class CAltQueue
{
public:
	CAltQueue(){ m_pItems[0] = m_pItems[1] = NULL;}
	~CAltQueue(){ Cleanup();}

public:
	void Create(int max_size)
	{
		m_pItems[0] = new int[max_size];
		m_pItems[1] = new int[max_size];
		m_nHeads[0] = m_nHeads[1] = 0;
		m_nTails[0] = m_nTails[1] = 0;
		m_nWrite = 0;
	}
	
	void Cleanup()
	{
		if( m_pItems[0] ) delete [] m_pItems[0];
		if( m_pItems[1] ) delete [] m_pItems[1];
	}

	void AddCurr(int item)
	{
		*(m_pItems[m_nWrite] + m_nTails[m_nWrite]++) = item;
	}

	void AddAlt(int item)
	{
		*(m_pItems[m_nWrite^1] + m_nTails[m_nWrite^1]++) = item;
	}
	
	BOOL PopCurr(int& item)
	{		
		if(m_nHeads[m_nWrite] == m_nTails[m_nWrite])
			return FALSE; 
	
		item = *(m_pItems[m_nWrite] + m_nHeads[m_nWrite]++);
		
		if(m_nHeads[m_nWrite] == m_nTails[m_nWrite])
		{
			m_nHeads[m_nWrite] = m_nTails[m_nWrite] = 0;			
		}

		return TRUE;
 	}

	void Alternate()
	{		
		m_nWrite ^= 1;
	}
	
	void ClearCurr()
	{
		m_nHeads[m_nWrite] = m_nTails[m_nWrite] = 0;	
	}

	void ClearAlt()
	{
		m_nHeads[m_nWrite^1] = m_nTails[m_nWrite^1] = 0;
	}
	
	BOOL IsCurrEmpty()
	{
		return m_nHeads[m_nWrite] == m_nTails[m_nWrite];
	}

	int CurrSize()
	{
		return m_nTails[m_nWrite] - m_nHeads[m_nWrite];
	}

	BOOL Peek(int idx, int& item)
	{
		if( m_nHeads[m_nWrite] + idx >= m_nTails[m_nWrite] ) return FALSE;
		item = *(m_pItems[m_nWrite] + m_nHeads[m_nWrite] + idx);
		return TRUE;
	}


public:  
	int*	m_pItems[2];
	int		m_nHeads[2];
	int		m_nTails[2];
	int		m_nWrite;
};

#endif //__ALT_QUEUE_H__BY_SGCHOI
