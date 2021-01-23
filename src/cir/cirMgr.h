/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.
#include "cirGate.h"
#include "cirDef.h"




struct STRCirGate
{
  size_t fanin1;
  size_t fanin2;
  bool inv1;
  bool inv2;
  STRCirGate(size_t f1, size_t f2, bool i1, bool i2)
  {
   fanin1 = f1;
   fanin2 = f2;
   inv1 = i1;
   inv2 = i2;
  }
};

struct STRCirGateHash
{
  size_t operator()(const STRCirGate& str) const{
    return ((size_t)str.fanin1 + (size_t)str.fanin2)/8;
  }
};
struct STRCirGateComp
{
  bool operator()(const STRCirGate& fanina, const STRCirGate& faninb) const {
    if ((fanina.fanin1 == faninb.fanin1) && (fanina.inv1 == faninb.inv1) &&
        (fanina.fanin2 == faninb.fanin2) && (fanina.inv2 == faninb.inv2)) return true;
    if ((fanina.fanin1 == faninb.fanin2) && (fanina.inv1 == faninb.inv2) &&
        (fanina.fanin2 == faninb.fanin1) && (fanina.inv2 == faninb.inv1)) return true;
    else return false;
  }
};

typedef unordered_map<STRCirGate, CirGate*, STRCirGateHash, STRCirGateComp> umap;


extern CirMgr *cirMgr;

class CirMgr
{
public:
   CirMgr(): _M(0), _I(0), _L(0), _O(0), _A(0), _doComment(0), _comment(""), _type("") {}
   ~CirMgr() { reset(); }

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const {
      if (_gatelist.find(gid) == _gatelist.end()) return 0;
      return _gatelist.find(gid)->second;
   }

  // Member functions about circuit DFS
  void genDFSList();

  // CONST 0 gate
  static CirGate* Const0;

  // reseting
  void reset();

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();
   Optimizegate opttype(CirGate*);
   int fanoutpin(CirGate*, CirGate*);
   int faninpin(CirGate*, CirGate*);
   void deletegate(CirGate*);
   void reduce_m_from_in(CirGate*, CirGate*);
   void reconnect(CirGate*, CirGate*, size_t, bool);

   void STRreconnect(CirGate*, CirGate*);
   size_t ptrtosize_t(CirGate*);

   //size_t howmanyzero(size_t);
   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

private:
   ofstream           *_simLog;
   // for parsi
   bool _readInitial(fstream&);
   bool _readPI(fstream&);
   bool _readPO(fstream&);
   bool _readAIG(fstream&);
   // bool _readSymbI(int , const string&);
   // bool _readSymbO(int, const string&);
   bool _readSymb(fstream&);
 
   bool _notSpace(char);
   bool _beSpace(char);
   bool _readNum(string&, int&, string);
 
   void _buildConnect();
   void _dfs(CirGate*);
 
 
 
   bool _doComment;
   string _comment;
   string _type;
 
   int _M, _I, _L, _O, _A;
   vector<CirPiGate*> _pilist;
   vector<CirPoGate*> _polist;
   vector<CirAigGate*> _aiglist;
   vector<CirGate*> _dfslist;
   map<unsigned, CirGate*> _gatelist;
};

#endif // CIR_MGR_H
