/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <unordered_map>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"
using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed



void
CirMgr::strash()
{
  //unordered_map<size_t, CirGate*> gatemap;
  umap gatemap;
  vector<CirGate*>::iterator dfsit;
  for (dfsit = _dfslist.begin();dfsit != _dfslist.end();++dfsit){
    if ((*dfsit)->_gateType == AIG_GATE){
      // const size_t kk = ptrtosize_t(*dfsit);
      // unordered_map<size_t, CirGate*>::iterator iter;
      // iter = gatemap.find(kk);
      // if (iter == gatemap.end()){
      //   gatemap.emplace(kk, (*dfsit));
      // }
      // else{
      //   //CirGate* preserve = gatemap[kk];
      //   STRreconnect((*iter).second, (*dfsit));
      //   cout << "Strashing: " << (*iter).second->_var << " merging " << (*dfsit)->_var << "..." << endl;
      // }
      STRCirGate qq((*dfsit)->_fanin[0].gate()->_var,(*dfsit)->_fanin[1].gate()->_var,(*dfsit)->_fanin[0].inv(),(*dfsit)->_fanin[1].inv());
      umap::iterator iter;
      iter = gatemap.find(qq);
      if (iter == gatemap.end()){
        gatemap.emplace(qq, (*dfsit));
      }
      else {
        STRreconnect((*iter).second, (*dfsit));
        cout << "Strashing: " << (*iter).second->_var << " merging " << (*dfsit)->_var << "..." << endl;
      }
    }
  }    
  _dfslist.clear();
  genDFSList();
  // cout << _dfslist.size() << endl;
  // cout << gg << endl;
}


void
CirMgr::fraig()
{

}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
