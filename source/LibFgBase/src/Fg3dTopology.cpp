//
// Copyright (c) 2015 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// Authors:     Andrew Beatty
// Created:     Dec. 14, 2009
//

#include "stdafx.h"

#include "FgStdSet.hpp"
#include "Fg3dTopology.hpp"
#include "FgValidVal.hpp"
#include "FgStdVector.hpp"

using namespace std;

FgVect2UI
Fg3dTopology::Tri::edge(uint relIdx) const
{
    if (relIdx == 0)
        return FgVect2UI(vertInds[0],vertInds[1]);
    else if (relIdx == 1)
        return FgVect2UI(vertInds[1],vertInds[2]);
    else if (relIdx == 2)
        return FgVect2UI(vertInds[2],vertInds[0]);
    else
        FGASSERT_FALSE;
    return FgVect2UI();
}

uint
Fg3dTopology::Edge::otherVertIdx(uint vertIdx) const
{
    if (vertIdx == vertInds[0])
        return vertInds[1];
    else if (vertIdx == vertInds[1])
        return vertInds[0];
    else
        FGASSERT_FALSE;
    return 0;       // make compiler happy
}

struct  EdgeVerts
{
    uint        loIdx;
    uint        hiIdx;

    EdgeVerts(uint i0,uint i1)
    {
        if (i0 < i1) {
            loIdx = i0;
            hiIdx = i1;
        }
        else if (i0 > i1) {
            loIdx = i1;
            hiIdx = i0;
        }
        else
            FGASSERT_FALSE;
    }

    // Comparison operator to use as a key for std::map:
    bool operator<(const EdgeVerts & rhs) const
    {
        if (loIdx < rhs.loIdx)
            return true;
        else if (loIdx == rhs.loIdx)
            return (hiIdx < rhs.hiIdx);
        else
            return false;
    }

    bool
    contains(uint idx) const
    {return ((idx == loIdx) || (idx == hiIdx)); }
};

struct  TriVerts
{
    FgVect3UI   inds;

    TriVerts(FgVect3UI i)
    {
        if (i[1] < i[0])
            std::swap(i[0],i[1]);
        if (i[2] < i[1])
            std::swap(i[1],i[2]);
        if (i[1] < i[0])
            std::swap(i[0],i[1]);
        inds = i;
    }

    bool operator<(const TriVerts & rhs) const
    {
        for (uint ii=0; ii<3; ++ii) {
            if (inds[ii] < rhs.inds[ii])
                return true;
            else if (inds[ii] == rhs.inds[ii])
                continue;
            else
                return false;
        }
        return false;
    }
};

Fg3dTopology::Fg3dTopology(
    const FgVerts &             verts,
    const vector<FgVect3UI> &   tris)
{
    // Remove null or duplicate tris so algorithms below work properly:
    uint                    duplicates = 0,
                            nulls = 0;
    m_tris.clear();
    set<TriVerts>           vset;
    for (size_t ii=0; ii<tris.size(); ++ii) {
        FgVect3UI           vis = tris[ii];
        if ((vis[0] == vis[1]) || (vis[1] == vis[2]) || (vis[2] == vis[0]))
            ++nulls;
        else {
            TriVerts            tv(vis);
            if (vset.find(tv) == vset.end()) {
                vset.insert(tv);
                Tri             tri;
                tri.vertInds = vis;
                m_tris.push_back(tri);
            }
            else
                ++duplicates;
        }
    }
    if (duplicates > 0)
        fgout << fgnl << "WARNING: Duplicate tris: " << duplicates;
    if (nulls > 0)
        fgout << fgnl << "WARNING: Null tris: " << nulls;
    // m_verts triInds
    // m_tris vertInds
    // edgesToTris
    m_verts.clear();
    m_verts.resize(verts.size());
    std::map<EdgeVerts,vector<uint> >    edgesToTris;
    for (size_t ii=0; ii<m_tris.size(); ++ii) {
        FgVect3UI       vertInds = m_tris[ii].vertInds;
        m_tris[ii].edgeInds = FgVect3UI(std::numeric_limits<uint>::max());
        for (uint jj=0; jj<3; ++jj) {
            m_verts[vertInds[jj]].triInds.push_back(uint(ii));
            EdgeVerts        edge(vertInds[jj],vertInds[(jj+1)%3]);
            edgesToTris[edge].push_back(uint(ii));
        }
    }
    // m_edges
    m_edges.clear();
    for (map<EdgeVerts,vector<uint> >::const_iterator it=edgesToTris.begin(); it!=edgesToTris.end(); ++it) {
        EdgeVerts       edgeVerts = it->first;
        Edge            edge;
        edge.vertInds = FgVect2UI(edgeVerts.loIdx,edgeVerts.hiIdx);
        edge.triInds = it->second;
        m_edges.push_back(edge);
    }
    // m_verts edgeInds:
    // m_tris edgeInds:
    for (size_t ii=0; ii<m_edges.size(); ++ii) {
        FgVect2UI               verts = m_edges[ii].vertInds;
        m_verts[verts[0]].edgeInds.push_back(uint(ii));
        m_verts[verts[1]].edgeInds.push_back(uint(ii));
        EdgeVerts                    edge(verts[0],verts[1]);
        const vector<uint> &    triInds = edgesToTris.find(edge)->second;
        for (size_t jj=0; jj<triInds.size(); ++jj) {
            uint                triIdx = triInds[jj];
            FgVect3UI           tri = m_tris[triIdx].vertInds;
            for (uint ee=0; ee<3; ++ee)
                if ((edge.contains(tri[ee]) && edge.contains(tri[(ee+1)%3])))
                    m_tris[triIdx].edgeInds[ee] = uint(ii);
        }
    }
    // validate:
    for (size_t ii=0; ii<m_verts.size(); ++ii) {
        const vector<uint> &    edgeInds = m_verts[ii].edgeInds;
        if (edgeInds.size() > 1)
            for (size_t jj=0; jj<edgeInds.size(); ++jj)
                m_edges[edgeInds[jj]].otherVertIdx(uint(ii));   // throws if index ii not found in edge verts
    }
    for (size_t ii=0; ii<m_tris.size(); ++ii)
        for (uint jj=0; jj<3; ++jj)
            FGASSERT(m_tris[ii].edgeInds[jj] != std::numeric_limits<uint>::max());
    for (size_t ii=0; ii<m_edges.size(); ++ii)
        FGASSERT(m_edges[ii].triInds.size() > 0);
}

FgVect2UI
Fg3dTopology::edgeFacingVertInds(uint edgeIdx) const
{
    const vector<uint> &    triInds = m_edges[edgeIdx].triInds;
    FGASSERT(triInds.size() == 2);
    uint        ov0 = oppositeVert(triInds[0],edgeIdx),
                ov1 = oppositeVert(triInds[1],edgeIdx);
    return FgVect2UI(ov0,ov1);
}

bool
Fg3dTopology::vertOnBoundary(uint vertIdx) const
{
    const vector<uint> &    edgeInds = m_verts[vertIdx].edgeInds;
    // If this vert is unused it is not on a boundary:
    for (size_t ii=0; ii<edgeInds.size(); ++ii) {
        const vector<uint> &    triInds = m_edges[edgeInds[ii]].triInds;
        if (triInds.size() == 1)
            return true;
    }
    return false;
}

vector<uint>
Fg3dTopology::vertBoundaryNeighbours(uint vertIdx) const
{
    vector<uint>            neighs;
    const vector<uint> &    edgeInds = m_verts[vertIdx].edgeInds;
    for (size_t ee=0; ee<edgeInds.size(); ++ee) {
        Edge                edge = m_edges[edgeInds[ee]];
        if (edge.triInds.size() == 1)
            neighs.push_back(edge.otherVertIdx(vertIdx));
    }
    return neighs;
}

vector<uint>
Fg3dTopology::vertNeighbours(uint vertIdx) const
{
    vector<uint>            ret;
    const vector<uint> &    edgeInds = m_verts[vertIdx].edgeInds;
    for (size_t ee=0; ee<edgeInds.size(); ++ee)
        ret.push_back(m_edges[edgeInds[ee]].otherVertIdx(vertIdx));
    return ret;
}

vector<vector<uint> >
Fg3dTopology::seams()
{
    vector<vector<uint> >   ret;
    vector<uint>            seam;
    vector<FgBool>          done(m_verts.size(),false);
    while (!(seam = findSeam(done)).empty())
        ret.push_back(seam);
    return ret;
}

vector<uint>
Fg3dTopology::findSeam(vector<FgBool> & done) const
{
    for (size_t ii=0; ii<done.size(); ++ii)
        if (!done[ii]) {
            vector<uint>    ret = traceSeam(done,uint(ii));
            if (!ret.empty())
                return ret;
        }
    return vector<uint>();
}

vector<uint>
Fg3dTopology::traceSeam(vector<FgBool> & done,uint vertIdx) const
{
    vector<uint>    ret;
    if (done[vertIdx])
        return ret;
    done[vertIdx] = true;
    if (vertOnBoundary(vertIdx)) {
        ret.push_back(vertIdx);
        vector<uint>    bns = vertBoundaryNeighbours(vertIdx);
        for (size_t ii=0; ii<bns.size(); ++ii)
            fgAppend(ret,traceSeam(done,bns[ii]));
    }
    return ret;
}

set<uint>
Fg3dTopology::traceFold(
    const Fg3dNormals & norms,
    vector<FgBool> &    done,
    uint                vertIdx)
    const
{
    set<uint>           ret;
    if (done[vertIdx])
        return ret;
    done[vertIdx] = true;
    const vector<uint> &    edgeInds = m_verts[vertIdx].edgeInds;
    for (size_t ii=0; ii<edgeInds.size(); ++ii) {
        const Edge &           edge = m_edges[edgeInds[ii]];
        if (edge.triInds.size() == 2) {         // Can not be part of a fold otherwise
            const Fg3dFacetNormals &    facetNorms = norms.facet[0];
            float       dot = fgDot(facetNorms.tri[edge.triInds[0]],facetNorms.tri[edge.triInds[1]]);
            if (dot < 0.5f) {                   // > 60 degrees
                ret.insert(vertIdx);
                fgAppend(ret,traceFold(norms,done,edge.otherVertIdx(vertIdx)));
            }
        }
    }
    return ret;
}

uint
Fg3dTopology::oppositeVert(uint triIdx,uint edgeIdx) const
{
    FgVect3UI       tri = m_tris[triIdx].vertInds;
    FgVect2UI       vertInds = m_edges[edgeIdx].vertInds;
    for (uint ii=0; ii<3; ++ii)
        if ((tri[ii] != vertInds[0]) && (tri[ii] != vertInds[1]))
            return tri[ii];
    FGASSERT_FALSE;
    return 0;
}

bool
Fg3dTopology::isManifold() const
{
    for (size_t ee=0; ee<m_edges.size(); ++ee) {
        Edge    edge = m_edges[ee];
        if (edge.triInds.size() == 1)
            break;
        if (edge.triInds.size() > 2)
            return false;
        Tri     tri0 = m_tris[edge.triInds[0]],
                tri1 = m_tris[edge.triInds[1]];
        uint    edgeIdx0 = fgFindFirstIdx(tri0.edgeInds,uint(ee)),
                edgeIdx1 = fgFindFirstIdx(tri1.edgeInds,uint(ee));
        bool    in0 = (tri0.edge(edgeIdx0) == edge.vertInds),
                in1 = (tri1.edge(edgeIdx1) == edge.vertInds);
        if (in0 && in1)
            return false;
        if (!in0 && !in1)
            return false;
    }
    return true;
}

size_t
Fg3dTopology::unusedVerts() const
{
    size_t      ret = 0;
    for (size_t ii=0; ii<m_verts.size(); ++ii)
        if (m_verts[ii].triInds.empty())
            ++ret;
    return ret;
}

// */
