// circtool.cpp : Defines the entry point for the console application.
//
#include "../util/typedefs.h"
#include "../util/config.h"
#include "../circuit/circuit.h"

#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <fstream>
using namespace std;
using namespace NTL;


void GenerateInput(int items)
{
	ostringstream os;

	os << "inputs";
	os << items;
	os << ".txt";

	ofstream fs1(os.str().c_str());  

	for(int i=0; i<items; i++)
	{
		fs1 << (items-i) << " ";
	}
	
	ostringstream os2;
	os2 << "inputc";
	os2 << items;
	os2 << ".txt";
	ofstream fs2(os2.str().c_str());  

	for(int i=0; i<items; i++)
	{
		fs2 << (i%2) << " ";
	}
}

void GenerateField()
{
	ZZ q = GenGermainPrime_ZZ(511);
	ZZ p = 2*q + 1;

	SetSeed(to_ZZ("72473209348201212167834"));

	ZZ gg =  RandomBnd(p);
	ZZ g;
	SqrMod(g, gg, p);

	ofstream fs("prime.txt");

	fs << "p" << endl << p << endl;
	fs << "g" << endl << g << endl;
}


int main(int argc, char** argv)
{
	double tt = clock();

	if( string("-g") == argv[1] )
	{
		GenerateField();
		return 0;
	}

	if( string("-c") == argv[1] )
	{ 
		CConfig* cf = new CConfig();
		cf->Load(argv[2]);
		CREATE_CIRCUIT(cf->GetNumParties(), cf->GetCircCreateName(), cf->GetCircCreateParams());
		return 0;
	}
	 
	if( argc < 4 )
	{
		cout << "circtool -r <config_file1> <config_file2> ... // run the circuit" << endl;
		cout << "circtool -rl <config_file1> <config_file2> ... // run the circuit with log (eval.txt)" << endl;
		cout << "circtool -st <config_file> <dst_circ_file> // save the circuit (text format)  " << endl;
		cout << "circtool -sb <config_file> <dst_circ_file> // save the circuit (binary format)  " << endl;
		cout << "circtool -tb <src_circ_txt_file> <dst_circ_bin_file> // convert into binary format " << endl;
		cout << "circtool -e <circ_txt_file> <circ_bin_file> // check equality" << endl;
		return 0;
	}

	if( string("-r") == argv[1] )
	{
		vector<const char*> configs(argc-2);
		for(int i=0; i<argc-2; i++)
		{
			configs[i] = argv[i+2];
		}

		TEST_CIRCUIT(configs, FALSE);
	}

	if( string("-rl") == argv[1] )
	{
		vector<const char*> configs(argc-2);
		for(int i=0; i<argc-2; i++)
		{
			configs[i] = argv[i+2];
		}

		TEST_CIRCUIT(configs, TRUE);
	}


	if( string("-st") == argv[1] || string("-sb") == argv[1] )
	{
	 	CConfig* cf = new CConfig();
		if(!cf->Load(argv[2]))
		{
			cout << "failure in opening the config file: " << argv[1] << endl;
			return 0;
		}
		 
		CCircuit* pCircuit = NULL;
		if( !cf->GetCircFileName().empty() )
		{
			cout << "creating circuit" << endl;

			pCircuit = LOAD_CIRCUIT_BIN(cf->GetCircFileName().c_str());

			if(!pCircuit)
			{
				cout << "failure in loading circuit " << cf->GetCircFileName() << endl;
				return 0;
			}
		}
		else
		{
			pCircuit = CREATE_CIRCUIT(cf->GetNumParties(), cf->GetCircCreateName(), cf->GetCircCreateParams() );
		}

		 
		cout << "saving circuit into " << argv[3]  << endl;

		if( string("-st") == argv[1] )
			pCircuit->Save(argv[3]);
		else
			pCircuit->SaveBin(argv[3]);
	} 
	
	if( string("-tb") == argv[1])
	{
		cout << "loading circuit" << endl;
		CCircuit* pCircuit = LOAD_CIRCUIT_TXT(argv[2]);

		cout << "saving circuit" << endl;
		pCircuit->SaveBin(argv[3]);
	
	}
	
	if (string("-e") == argv[1])
	{
		cout << "loading circuit " << argv[2]  << endl;
		CCircuit* pCircuit = LOAD_CIRCUIT_TXT(argv[2]);

		cout << "loading circuit " << argv[3]  << endl;
		CCircuit* pCircuit2 = LOAD_CIRCUIT_BIN(argv[3]);

		BOOL b = pCircuit->IsEqual(pCircuit2);
		cout << "equality = " << (b? "true":"false") << endl;
	}


	double tt1 = clock();
	cout << endl << "elapsed " <<  (tt1-tt)/CLOCKS_PER_SEC << " seconds." << endl;

	return 0;
}
