//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2014 Ulrich von Zadow
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  Current versions can be found at www.libavg.de
//

#include "DAG.h"

#include "Exception.h"

#include <boost/enable_shared_from_this.hpp>

using namespace std;

namespace avg {

class AVG_API DAGNode: public boost::enable_shared_from_this<DAGNode>
{
public:
    DAGNode(long vertexID, const std::set<long>& outgoingIDs);
    void resolveIDs(DAG* pDAG);

    long m_VertexID;
    std::set<long> m_OutgoingIDs;
    std::set<DAGNodePtr> m_pOutgoingNodes;
    std::set<DAGNodePtr> m_pIncomingNodes;
};


DAGNode::DAGNode(long vertexID, const set<long>& outgoingIDs)
{
    m_VertexID = vertexID;
    m_OutgoingIDs = outgoingIDs;
}

void DAGNode::resolveIDs(DAG* pDAG)
{
    set<long>::iterator it;
    for (it=m_OutgoingIDs.begin(); it!=m_OutgoingIDs.end(); ++it) {
        long outgoingID = *it;
        DAGNodePtr pDestNode = pDAG->findNode(outgoingID);
        m_pOutgoingNodes.insert(pDestNode);
        pDestNode->m_pIncomingNodes.insert(shared_from_this());
    }
    m_OutgoingIDs.clear();
}


DAG::DAG()
{
}

DAG::~DAG()
{
}

void DAG::addNode(long vertexID, const set<long>& outgoingIDs)
{
    DAGNode* pNode = new DAGNode(vertexID, outgoingIDs);
    m_pNodes.insert(DAGNodePtr(pNode));
}

void DAG::sort(vector<long>& pResults)
{
    resolveIDs();
    while (!m_pNodes.empty()) {
        DAGNodePtr pCurNode = findStartNode(*m_pNodes.begin());
        removeNode(pCurNode);
        pResults.push_back(pCurNode->m_VertexID);
    }
}

void DAG::resolveIDs()  
{
    set<DAGNodePtr>::iterator it;
    for (it=m_pNodes.begin(); it!=m_pNodes.end(); ++it) {
        (*it)->resolveIDs(this);
    }
}

DAGNodePtr DAG::findNode(long id)
{
    set<DAGNodePtr>::iterator it;
    for (it=m_pNodes.begin(); it!=m_pNodes.end(); ++it) {
        if ((*it)->m_VertexID == id) {
            return (*it);
        }
    }
    AVG_ASSERT(false);
    return DAGNodePtr();
}

void DAG::removeNode(DAGNodePtr pNode)
{
    set<DAGNodePtr>::iterator it;
    for (it=pNode->m_pOutgoingNodes.begin(); it!=pNode->m_pOutgoingNodes.end(); ++it) {
        DAGNodePtr pDestNode = *it;
        pDestNode->m_pIncomingNodes.erase(pNode);
    }
    m_pNodes.erase(pNode);
}

DAGNodePtr DAG::findStartNode(DAGNodePtr pNode, unsigned depth)
{
    if (pNode->m_pIncomingNodes.empty()) {
        return pNode;
    } else {
        if (depth > m_pNodes.size()) {
            throw Exception(AVG_ERR_INVALID_ARGS, "cyclic graph");
        }
        return findStartNode(*pNode->m_pIncomingNodes.begin(), depth+1);
    }
}

}
