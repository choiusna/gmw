// main.cpp
#include "../util/typedefs.h"
#include "../util/config.h"
#include "../circuit/circuit.h"

#include "party.h"
#include <ctime>
using namespace std;

void PrintOutput(const vector<int>& vOutput) 
{
	cout << "output:" << endl;
	cout << "(binary)";
	for( UINT i=0; i<vOutput.size(); i++ )
	{
		cout << " " << (int) vOutput[i];
	}
	cout << endl;

	ZZ out;
	out.SetSize(vOutput.size());
	
	cout << "(numeric:big-endian) ";

	int size = vOutput.size();
	for( int i=0; i<size; i++ )
	{
		if( bit(out,i) != vOutput[i] )
			SwitchBit(out,i);
	}
	cout << out << endl;


	cout << "(numeric:little-endian) ";

	for( int i=0; i<size; i++ )
	{
		if( bit(out,i) != vOutput[size-i-1] )
			SwitchBit(out,i);
	}
	cout << out << endl;
}


int main(int argc, char** argv)
{
	if( argc != 2 )
	{
		cout << "usage: mpc.exe config_file " << endl;
		return 0;
	}

	double tt = clock();

	CConfig* pConfig = new CConfig();
	if(!pConfig->Load(argv[1]))
	{
		cout << "failure in opening the config file: " << argv[1] << endl;
		return 0;
	}

	CParty* pParty = new CParty();
	pParty->Run();
	PrintOutput( pParty->GetOutput() );
	 
	delete pParty; 
 	 
	double tt1 = clock();
	cout << endl << "elapsed " <<  (tt1-tt)/CLOCKS_PER_SEC << " seconds." << endl;

	 
	return 0;
}

