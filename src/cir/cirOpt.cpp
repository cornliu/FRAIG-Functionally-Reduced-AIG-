/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
  map<unsigned, CirGate*>::iterator iter;
  for (iter = _gatelist.begin();iter != _gatelist.end();++iter){
    if ((*iter).second->_inDFSlist == false){
      if ((*iter).second->getType() == AIG_GATE){
        vector<CirAigGate*>::iterator i;
        for (i = _aiglist.begin();i != _aiglist.end();i++){
          if ((*i)->_var == (*iter).second->_var){
            cout << "AIG" << (*i)->_var << endl;
            _aiglist.erase(i);
            _gatelist.erase(iter);
            break;
          }
        }
      }
      else if ((*iter).second->getType() == UNDEF_GATE){
        cout << "UNDEF" << (*iter).second->_var << endl;
        _gatelist.erase(iter);
      }
    }
  }
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
  int gg = 0;
  vector<CirGate*>::iterator iter;
  for (iter = _dfslist.begin();iter != _dfslist.end();++iter){
    if ((*iter)->_gateType == AIG_GATE){
      Optimizegate type = opttype(*iter);
      if (type == DependOnRight){
        cout << "Simplifying: " << (*iter)->_fanin[1].gate()->_var << " merging ";
        if ((*iter)->_fanin[1].inv()) cout << "!";
        cout << (*iter)->_var << "..." << endl;
        CirGate* gatein = (*iter)->_fanin[1].gate();
        bool ininv = (*iter)->_fanin[1].inv();
        reduce_m_from_in((*iter), gatein);
        for (size_t i = 0;i < (*iter)->_fanout.size();++i){
          CirGate* gateout = (*iter)->_fanout[i].gate();
          size_t pin = fanoutpin((*iter)->_fanout[i].gate(), (*iter));
          reconnect(gateout, gatein, pin, ininv);
        }
        deletegate(*iter);
      }
      else if (type == DependOnLeft){
        cout << "Simplifying: " << (*iter)->_fanin[0].gate()->_var << " merging ";
        if ((*iter)->_fanin[0].inv()) cout << "!";
        cout << (*iter)->_var << "..." << endl;
        CirGate* gatein = (*iter)->_fanin[0].gate();
        bool ininv = (*iter)->_fanin[0].inv();
        reduce_m_from_in((*iter), gatein);
        for (size_t i = 0;i < (*iter)->_fanout.size();++i){
          CirGate* gateout = (*iter)->_fanout[i].gate();
          size_t pin = fanoutpin((*iter)->_fanout[i].gate(), (*iter));
          reconnect(gateout, gatein, pin, ininv);
        }
        deletegate(*iter);
      }
      else if (type == Cond){
        for (size_t i = 0;i < (*iter)->_fanout.size();++i){
          (*iter)->_fanout[i].gate()->_fanin[fanoutpin((*iter)->_fanout[i].gate() ,(*iter))].cgate(Const0);
          Const0->_fanout.push_back((*iter)->_fanout[i]);
        }
        deletegate(*iter);
        cout << "Simplifying: " << "0" << " merging " << (*iter)->_var << "..." << endl;
      }

    }
  }
  _dfslist.clear();
  genDFSList();
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
