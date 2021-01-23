/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;
CirGate* CirMgr::Const0 = new CirGate(0, 0, CONST_GATE);

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static CirParseError missError;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
   fstream file(fileName.c_str());
   if (!file) { cerr << "Cannot open design \"" << fileName << "\"!!" << endl; return false; }
   
   if (!_readInitial(file)) return false; // will get _type _M _I _L _O _A
   if (!_readPI(file)) return false; // get PIs
   if (!_readPO(file)) return false; // get POs
   if (!_readAIG(file)) return false; // get AIGs
   if (!_readSymb(file)) return false; // read symb
   if (_doComment) {
      char ch;
      while (file.get(ch)) _comment += ch;
   }
   file.close();

   // build connect
   cirMgr->_buildConnect();
   return true;
}
bool CirMgr::_notSpace(char ch) {
   if (isspace(ch)) {
      if (ch == ' ') return parseError(EXTRA_SPACE);
      else { 
         errInt = int(ch);
         return parseError(ILLEGAL_WSPACE);
      }
   }
   return true;
}

bool CirMgr::_beSpace(char ch) {
   if (ch != ' ') return parseError(MISSING_SPACE);
   return true;
}

bool CirMgr::_readNum(string& line, int& num, string err) {
   string numStr = "";
   if (!_notSpace(line[colNo])) return false;
   for (unsigned s = colNo; s < line.size() && !isspace(line[s]); ++s) numStr += line[s];
   if (numStr == "") { errMsg = err; return parseError(missError); }
   for (auto i : numStr) if (!isdigit(i)) {
      errMsg = err +"(" + numStr + ")";
      return parseError(ILLEGAL_NUM);
   }
   num = stoi(numStr);
   colNo += numStr.size();
   return true;
}

bool CirMgr::_readInitial(fstream& file) {
   string line = "";
   getline(file, line);
   if (line.size() == 0) { errMsg = "aag"; return parseError(MISSING_IDENTIFIER); }
   colNo = 0;
   if (!_notSpace(line[0])) return false;
   // check _type
   for (colNo = 0; isalpha(line[colNo]); ++colNo) _type += line[colNo];
   if (_type != "aag") { errMsg = _type; return parseError(ILLEGAL_IDENTIFIER); }
   // check ' M I L O A'
   // read M
   missError = MISSING_NUM;
   if (!_beSpace(line[colNo])) return false;
   colNo += 1;
   if (!_readNum(line, _M, "number of variables")) return false;
   // read I
   if (!_beSpace(line[colNo])) return false;
   colNo += 1;
   if (!_readNum(line, _I, "number of PIs")) return false;
   // read L
   if (!_beSpace(line[colNo])) return false;
   colNo += 1;
   if (!_readNum(line, _L, "number of latches")) return false;
   // read O
   if (!_beSpace(line[colNo])) return false;
   colNo += 1;
   if (!_readNum(line, _O, "number of POs")) return false;
   // read A
   if (!_beSpace(line[colNo])) return false;
   colNo += 1;
   if (!_readNum(line, _A, "number of AIGs")) return false;
   if (colNo < line.size()) return parseError(MISSING_NEWLINE);

   if (_M < _I + _L + _A) {
      errMsg = "Number of variables"; errInt = _M;
      return parseError(NUM_TOO_SMALL);
   }
   if (_L != 0) {
      cerr << "[ERROR] Line 1: Illegal latches!!" << endl;
      return false;
   }

   ++lineNo;
   colNo = 0;
   return true;
}

bool 
CirMgr::_readPI(fstream& file) {
   int repeat = _I;
   missError = MISSING_DEF;
   while (repeat--) {
      int lit;
      string line;
      if (!getline(file, line)) { errMsg = "PI"; return parseError(MISSING_DEF); }
      if (!_readNum(line, lit, "PI")) return false;
      if (colNo < line.size()) return parseError(MISSING_NEWLINE);
      errInt = lit;
      if (lit / 2 == 0) return parseError(REDEF_CONST);
      if (lit % 2) {
         errMsg = "PI";
         return parseError(CANNOT_INVERTED);
      }
      if (lit / 2 > _M) return parseError(MAX_LIT_ID);

      CirPiGate* newPi = new CirPiGate(lit, lineNo + 1);
      for (auto g : _pilist) if (g->getVar() == lit / 2) {
         errGate = g;
         return parseError(REDEF_GATE);
      }
      _pilist.push_back(newPi);
      _gatelist[newPi->getVar()] = newPi;
      ++lineNo;
      colNo = 0;
   }

   return true;
}

bool 
CirMgr::_readPO(fstream& file) {
   missError = MISSING_DEF;
   for (int re = 1; re <= _O; ++re) {
      int lit;
      string line;
      if (!getline(file, line)) { errMsg = "PO"; return parseError(MISSING_DEF); }
      if (!_readNum(line, lit, "PO")) return false;
      if (colNo < line.size()) return parseError(MISSING_NEWLINE);
      errInt = lit;
      if (lit / 2 > _M) return parseError(MAX_LIT_ID);

      CirPoGate* newPo = new CirPoGate(lit, re + _M, lineNo + 1);
      for (auto g : _polist) if (g->getVar() == lit / 2) {
         errGate = g;
         return parseError(REDEF_GATE);
      }
      _polist.push_back(newPo);
      _gatelist[newPo->getVar()] = newPo;
      ++lineNo;
      colNo = 0;
   }

   return true;
}

bool 
CirMgr::_readAIG(fstream& file) {
   int repeat = _A;
   missError = MISSING_DEF;
   while (repeat--) {
      int lit, src1, src2;
      string line;
      if (!getline(file, line)) { errMsg = "AIG"; return parseError(MISSING_DEF); }
      // read AIG lit
      if (!_readNum(line, lit, "AIG")) return false;
      errInt = lit;
      if (lit / 2 == 0) return parseError(REDEF_CONST);
      if (lit % 2) {
         errMsg = "AIG";
         return parseError(CANNOT_INVERTED);
      }
      if (lit / 2 > _M) return parseError(MAX_LIT_ID);

      // read AIG src1
      if (!_beSpace(line[colNo])) return false;
      colNo += 1;
      if (!_readNum(line, src1, "AIG")) return false;
      if (src1 / 2 > _M) { errInt = src1; return parseError(MAX_LIT_ID); }
      // read AIG src2
      if (!_beSpace(line[colNo])) return false;
      colNo += 1;
      if (!_readNum(line, src2, "AIG")) return false;
      if (src2 / 2 > _M) { errInt = src2; return parseError(MAX_LIT_ID); }
      if (colNo < line.size()) return parseError(MISSING_NEWLINE);

      CirAigGate* newAig = new CirAigGate(lit, src1, src2, lineNo + 1);
      for (auto g : _aiglist) if (g->getVar() == lit / 2) {
         errGate = g;
         return parseError(REDEF_GATE);
      }
      for (auto g : _pilist) if (g->getVar() == lit / 2) {
         errGate = g;
         return parseError(REDEF_GATE);
      }
      _aiglist.push_back(newAig);
      _gatelist[newAig->getVar()] = newAig;
      ++lineNo;
      colNo = 0;
   }

   return true;
}

bool CirMgr::_readSymb(fstream& file) {
   string line = "";
   while (getline(file, line)) {
      char type; int pos; string symb;
      if (!_notSpace(line[0])) { return false; }
      type = line[0];
      if (type == 'c') break;
      if (type != 'i' && type != 'l' && type != 'o') {
         errMsg = type; return parseError(ILLEGAL_SYMBOL_TYPE);
      }
      colNo += 1;

      if (!_readNum(line, pos, "symbol index")) return false;
      if (colNo == line.size()) {
         errMsg = "symbolic name";
         return parseError(MISSING_IDENTIFIER);
      }
      
      if (!_beSpace(line[colNo])) { return false; }
      colNo += 1;
      symb = line.substr(colNo);
      if (symb.size() == 0) {
         errMsg = "symbolic name";
         return parseError(MISSING_IDENTIFIER);
      }
      for (size_t i = 0; i < symb.size(); ++i) if (!isprint(symb[i])) {
         errInt = int(symb[i]);
         colNo += i;
         return parseError(ILLEGAL_SYMBOL_NAME);
      }
      switch (type) {
         case 'i':
            if (pos >= _pilist.size()) {
               errMsg = "PI index"; errInt = pos;
               return parseError(NUM_TOO_BIG);
            }
            if (_pilist[pos]->_symbo.size()) {
               errMsg = 'i'; errInt = pos;
               return parseError(REDEF_SYMBOLIC_NAME);
            }
            _pilist[pos]->addSymbol(symb);
            break;
         case 'o':
            if (pos >= _polist.size()) {
               errMsg = "PO index"; errInt = pos;
               return parseError(NUM_TOO_BIG);
            }
            if (_polist[pos]->_symbo.size()) {
               errMsg = 'o'; errInt = pos;
               return parseError(REDEF_SYMBOLIC_NAME);
            }
            _polist[pos]->addSymbol(symb);
            break;
         default:
            return false;
      }
      ++lineNo;
      colNo = 0;
   }
   // get "c" or nothing
   if (line.size()) {
       colNo += 1;
       if (colNo != line.size()) return parseError(MISSING_NEWLINE);
       _doComment = true;
   }
   return true;
}

int 
CirMgr::fanoutpin(CirGate* gate1, CirGate* gate2){
   for (size_t i = 0;i < gate1->_fanin.size();i++){
      if (gate1->_fanin[i].gate()->_var == gate2->_var) return i;
   }
   // if (gate1->_fanin[0].gate()->_var == gate2->_var) return 0;
   // else if (gate1->_fanin[1].gate()->_var == gate2->_var) return 1;
}

int 
CirMgr::faninpin(CirGate* gate1, CirGate* gate2){
   for (size_t n = 0;n < gate2->_fanout.size();n++){
      if (gate2->_fanout[n].gate()->_var == gate1->_var){
         return n;
      }
   }
}
void 
CirMgr::deletegate(CirGate* gate){
   assert(gate->_gateType == AIG_GATE);
   vector<CirAigGate*>::iterator i;
   for (i = _aiglist.begin();i != _aiglist.end();++i){
      if ((*i)->_var == gate->_var) {
         _aiglist.erase(i); 
         break;
      }
   }
   map<unsigned, CirGate*>::iterator a;
   for (a = _gatelist.begin();a != _gatelist.end();++a){
      if ((*a).second->_var == gate->_var) {
         _gatelist.erase(a); 
         break;
      }
   }
}
Optimizegate
CirMgr::opttype(CirGate* gate) 
{
   assert(gate->_gateType == AIG_GATE);
   size_t t0 = gate->_fanin[0].gate()->_var ,t1 = gate->_fanin[1].gate()->_var;
   if ((t0 == t1) && (gate->_fanin[0].inv() == gate->_fanin[1].inv())) return DependOnRight;
   else if ((t0 == t1) && (gate->_fanin[0].inv() != gate->_fanin[1].inv())) return Cond;
   else if ((t0 == 0) && (gate->_fanin[0].inv())) return DependOnRight;
   else if ((t1 == 0) && (gate->_fanin[1].inv())) return DependOnLeft;
   if ((t0 == 0) && (gate->_fanin[0].inv() == false)) return Cond;
   if ((t1 == 0) && (gate->_fanin[1].inv() == false)) return Cond;
   return Nothing;
}

void
CirMgr::reduce_m_from_in(CirGate* mgate, CirGate* ingate)
{
   vector<CirGateV>::iterator it;
   for (it = ingate->_fanout.begin();it != ingate->_fanout.end();++it){
      if ((*it).gate()->_var == mgate->_var) {
         ingate->_fanout.erase(it);
         return;
      }
   }
}
void
CirMgr::reconnect(CirGate* gateout, CirGate* gatein ,size_t pin, bool inv)
{
   if (gateout->_fanin[pin].inv() == inv){
      gateout->_fanin[pin].cinv(false);
      gateout->_fanin[pin].cgate(gatein);
      CirGateV gg(gateout, false);
      gatein->_fanout.push_back(gg);
   }
   else{
      gateout->_fanin[pin].cinv(true);
      gateout->_fanin[pin].cgate(gatein);
      CirGateV gg(gateout, true);
      gatein->_fanout.push_back(gg);
   }
}

void
CirMgr::STRreconnect(CirGate* preserve, CirGate* merged){
   vector<CirGateV>::iterator iter;
   for (iter = merged->_fanout.begin();iter != merged->_fanout.end();++iter){
      preserve->_fanout.push_back(*iter);
      (*iter).gate()->_fanin[fanoutpin((*iter).gate(), merged)].cgate(preserve);
   }
   for (int i = 0;i < merged->_fanin.size();++i){
      reduce_m_from_in(merged, merged->_fanin[i].gate());
   }
   deletegate(merged);
}

size_t
CirMgr::ptrtosize_t(CirGate* gate){
  
  size_t tt = 0;
  vector<CirGateV>::iterator iter;
  for (iter = gate->_fanin.begin();iter != gate->_fanin.end();++iter){
     tt += (size_t)iter->gate()>>(size_t)iter->inv();
  }
  return tt;
//   int a = gate->_fanin[0].gate()->_var;
//   int b = gate->_fanin[1].gate()->_var;
//   if (gate->_fanin[0].inv() == true){
//      a = a*2+1;
//   }
//   else {
//      a = a*2;
//   }
//   if (gate->_fanin[1].inv() == true){
//      b = b*2+1;
//   }
//   else{
//      b = b*2;
//   }
//   if (a < b){
//      return a*howmanyzero(b) + b;
//   }
//   else{
//      return b*howmanyzero(a) + a;
//   }
   

}
// size_t
// CirMgr::howmanyzero(size_t number){
//    size_t a = 0;
//    while(true){
//       a+=1;
//       number = number/10;
//       if (number/10 == 0){
//          break;
//       }
//    }
//    return a;
// }

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
   cout << endl;
   cout << "Circuit Statistics" << endl;
   cout << "==================" << endl;
   cout << "  PI" << right << setw(12) << _pilist.size() << endl;
   cout << "  PO" << right << setw(12) << _polist.size() << endl;
   cout << "  AIG" << right << setw(11) << _aiglist.size() << endl;
   cout << "------------------" << endl;
   cout << "  Total" << right << setw(9) << _pilist.size() + _polist.size() + _aiglist.size() << endl;

}

void
CirMgr::printNetlist() const
{
/*
   cout << endl;
   for (unsigned i = 0, n = _dfsList.size(); i < n; ++i) {
      cout << "[" << i << "] ";
      _dfsList[i]->printGate();
   }
*/
   cout << endl;
   for (size_t i = 0;i < _dfslist.size(); ++i) {
      CirGate* g = _dfslist[i];
      cout << "[" << i << "] ";
      cout << left << setw(4) << g->getTypeStr() << g->_var;
      for (size_t j = 0;j < g->_fanin.size(); ++j) {
         cout << " " << (g->_fanin[j].gate()->_gateType == UNDEF_GATE ? "*" : "") \
            << (g->_fanin[j].inv() ? "!" : "") << g->_fanin[j].gate()->_var;
      }
      if (g->_symbo != "") cout << " (" << g->_symbo << ")";
      cout << endl;
   }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for (size_t i = 0;i < _pilist.size(); ++i) 
      cout << " " << _pilist[i]->getVar();
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for (size_t i = 0;i < _polist.size(); ++i) 
      cout << " " << _polist[i]->getVar();
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
   vector<unsigned> fltFanins;
   vector<unsigned> notUsed;
   for (map<unsigned, CirGate*>::const_iterator it = _gatelist.begin(); it != _gatelist.end(); ++it) {
      CirGate* gate = it->second;
      if (gate->getType() == CONST_GATE || gate->getType() == UNDEF_GATE) continue;
      for (auto i : gate->_fanin) {
         if (i.gate()->getType() == UNDEF_GATE) {
            fltFanins.push_back(gate->getVar());
            break;
         }
      }

      if ((gate->_fanout).empty() && gate->getType() != PO_GATE) notUsed.push_back(gate->getVar());
   }

   if (!fltFanins.empty()) {
      cout << "Gates with floating fanin(s):";
      for (size_t i = 0;i < fltFanins.size(); ++i)
         cout << " " << fltFanins[i];
      cout << endl;
   }
   if (!notUsed.empty()) {
      cout << "Gates defined but not used  :";
      for (size_t i = 0;i < notUsed.size(); ++i)
         cout << " " << notUsed[i];
      cout << endl;
   }
}

void
CirMgr::printFECPairs() const
{
}

void
CirMgr::writeAag(ostream& outfile) const
{
   outfile << _type << " " << _M << " " << _I << " " \
      << _L << " " << _O << " ";
   // count AIG in _dfslist
   int validA = 0;
   for (auto i : _dfslist) if (i->_gateType == AIG_GATE) ++validA;
   outfile << validA << endl;

   for (auto i : _pilist) outfile << i->_var * 2 << endl;
   for (auto i : _polist) {
      outfile << i->_fanin[0].gate()->_var * 2 + int(i->_fanin[0].inv()) << endl;
   }
   for (auto i : _dfslist) {
      if (i->_gateType == AIG_GATE) {
         outfile << i->_var * 2;
         for (size_t j = 0;j < i->_fanin.size(); ++j) {
            outfile << " " << i->_fanin[j].gate()->_var * 2 + int(i->_fanin[j].inv());
         }
         outfile << endl;
      }
   }
   for (size_t i = 0;i < _pilist.size(); ++i) {
      if (_pilist[i]->_symbo.size()) outfile << "i" << i << " " << _pilist[i]->_symbo << endl;
   }
   for (size_t i = 0;i < _polist.size(); ++i) {
      if (_polist[i]->_symbo.size()) outfile << "o" << i << " " << _polist[i]->_symbo << endl;
   }
   string myComment = "AAG output by Che-Kuang (Ken) Chu";
   outfile << "c\n" << myComment << endl;
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
   
}

void
CirMgr::_buildConnect() {
   _gatelist[0] = Const0;
   for (map<unsigned, CirGate*>::iterator it = _gatelist.begin(); it != _gatelist.end(); ++it) {
      if (it->second->_gateType == PO_GATE || it->second->_gateType == AIG_GATE) it->second->connect(_gatelist);
   }
   genDFSList();
}

void
CirMgr::genDFSList() {
   CirGate::setGlobalRef();
   for (size_t i = 0;i < _polist.size(); ++i) _dfs(_polist[i]);
}

void
CirMgr::_dfs(CirGate* gate) {
   gate->setToGlobalRef();
   for (size_t i = 0;i < gate->_fanin.size(); ++i) {
      if (!gate->_fanin[i].gate()->isGlobalRef()) {
         _dfs(gate->_fanin[i].gate());
      }
   }
   if (gate->_gateType != UNDEF_GATE) {
      gate->_inDFSlist = true;
      _dfslist.push_back(gate);
   }
}

void
CirMgr::reset() {
   _pilist.clear();
   _polist.clear();
   _aiglist.clear();
   _dfslist.clear();
   for (map<unsigned, CirGate*>::iterator it = _gatelist.begin(); it != _gatelist.end(); ++it) {
      delete it->second;
   }
   _gatelist.clear();
   CirMgr::Const0 = new CirGate(0, 0, CONST_GATE);

   _M = _I = _L = _O = _A = 0;
   _doComment = 0;
   _comment = _type = "";

   lineNo = 0;
   colNo = 0;
}

