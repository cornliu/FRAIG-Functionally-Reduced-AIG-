/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;
class CirPiGate;
class CirPoGate;
class CirAigGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------

class CirGateV {
  public:
  friend class CirGate;
  friend class CirPiGate;
  friend class CirPoGate;
  friend class CirAigGate;

  CirGateV() {}
  CirGateV(CirGate* g, bool inv): _gate(g), _inv(inv) {}
  CirGate* gate() const { return _gate; }
  bool inv() const { return _inv; }
  void cinv(bool flag) { _inv = flag; }
  void cgate(CirGate* ttt){ _gate = ttt; }
  protected:
  CirGate* _gate;
  bool _inv;
};

class CirGate
{
public:
  CirGate() {}
  CirGate(int var, int lineNo, GateType gateType) {
    _var = var; _lineNo = lineNo; _gateType = gateType;
    _inDFSlist = false;
  }
  virtual ~CirGate() { reset(); }

  friend class CirMgr;

  // Basic access methods
  string getTypeStr() const {
    switch(_gateType) {
      case UNDEF_GATE: return "UNDEF";
      case PI_GATE: return "PI";
      case PO_GATE: return "PO";
      case AIG_GATE: return "AIG";
      case CONST_GATE: return "CONST";
      case TOT_GATE: return "TOT_GATE";
      default: return "";
    }
  }
  unsigned getLineNo() const { return _lineNo; }
  virtual bool isAig() const { return false; }

  // Printing functions
  // virtual void printGate() const = 0;
  void printGate() {} // this is I add
  void reportGate() const;
  void reportFanin(int level);
  void reportFanout(int level);

  unsigned getVar() { return _var; }
  GateType getType() { return _gateType; }

  void connect(map<unsigned, CirGate*>&);

  void addSymbol(const string& symb) {
    _symbo = symb;
  }

  bool isGlobalRef() { return _ref == _globalRef; }
  void setToGlobalRef() { _ref = _globalRef; }
  static void setGlobalRef() { ++_globalRef; }

  void reset();

  bool inDFSlist(){ return _inDFSlist; }

private:
  static unsigned _globalRef;
  unsigned _ref;

  void _dfsFanin(CirGate*, unsigned, bool, int);
  void _dfsFanout(CirGate*, unsigned, bool, int);

protected:
  GateType _gateType;
  unsigned _var;
  unsigned _lineNo;
  vector<CirGateV> _fanin;
  vector<CirGateV> _fanout;
  // vector<CirGate*> _fanin;
  // vector<CirGate*> _fanout;
  // vector<bool> _inv; // input inverse
  // vector<bool> _outv; // output inverse
  string _symbo;

  bool _inDFSlist;
};

class CirPiGate : public CirGate
{
public:
  CirPiGate() {}
  CirPiGate(int lit, unsigned lineNo) {
    _gateType = PI_GATE;
    _lineNo = lineNo;
    _var = lit / 2;
    _symbo = "";
    _inDFSlist = false;
  }
  ~CirPiGate() { reset(); }
};

class CirPoGate : public CirGate
{
public:
  CirPoGate() {}
  CirPoGate(int srclit, int var, unsigned lineNo) {
    _gateType = PO_GATE;
    _lineNo = lineNo;
    _var = var;
    _inDFSlist = false;

    // _inv.push_back(srclit % 2 == 1 ? 1 : 0);

    size_t srcVar = (size_t)(srclit / 2);
    // _fanin.push_back((CirGate*)srcVar);
    _fanin.emplace_back((CirGate*)srcVar, srclit % 2 == 1 ? 1 : 0);
    _symbo = "";
  }
  ~CirPoGate() { reset(); }
};

class CirAigGate : public CirGate
{
public:
  CirAigGate() {}
  CirAigGate(int lit, int src1, int src2, unsigned lineNo) {
    _gateType = AIG_GATE;
    _lineNo = lineNo;
    _var = lit / 2;
    _inDFSlist = false;

    // _inv.push_back(src1 % 2 == 1 ? 1 : 0);
    // _inv.push_back(src2 % 2 == 1 ? 1 : 0);

    size_t var1 = (size_t)(src1 / 2);
    size_t var2 = (size_t)(src2 / 2);
    // _fanin.push_back((CirGate*)var1);
    // _fanin.push_back((CirGate*)var2);
    _fanin.emplace_back((CirGate*)var1, src1 % 2 == 1 ? 1 : 0);
    _fanin.emplace_back((CirGate*)var2, src2 % 2 == 1 ? 1 : 0);
    _symbo = "";
  }
  ~CirAigGate() { reset(); }
};

// class CirGate
// {
// public:
//    CirGate(){}
//    virtual ~CirGate() {}

//    // Basic access methods
//    string getTypeStr() const { return ""; }
//    unsigned getLineNo() const { return 0; }
//    virtual bool isAig() const { return false; }

//    // Printing functions
//    virtual void printGate() const {}
//    void reportGate() const;
//    void reportFanin(int level) const;
//    void reportFanout(int level) const;
   
// private:

// protected:
// };

#endif // CIR_GATE_H
