//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2006 Ulrich von Zadow
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
//  Original author of this file is igor@c-base.org 
//

#include "DeDistort.h"

#include "../base/XMLHelper.h"

#include <cstring>
#include <iostream>
#include <math.h>

const double sqrt3 = sqrt(3.);

using namespace std;

namespace avg {

// This transformation is based on the undistort code found at 
// http://www.math.rutgers.edu/~ojanen/undistort/index.html. 
//   a lot of parameters enter here, some of which can be calculated/set manually, 
//   some of which need to be determined via an optimization procedure
//
//   * m_CamExtents is the size of the camera image. This is used to transform the 
//     coordinates of the image so they fall in the range (-1,-1)-(1,1)
//   * m_pDistortionParams OPT see paper
//   * m_Angle corrects rotation of camera OPT
//   * m_DisplayScale convert back from standard coords to display
//   * m_DisplayOffset correct the offset of the display from the center of the table
//     

DeDistort::DeDistort()
    : m_CamExtents(1,1),
      m_Angle(0.0),
      m_TrapezoidFactor(0),
      m_DisplayOffset(0,0),
      m_DisplayScale(1,1)
{
    m_DistortionParams.push_back(0);
    m_DistortionParams.push_back(0);
    m_RescaleFactor = calc_rescale();
}

DeDistort::DeDistort(const DPoint& CamExtents, const DPoint& DisplayExtents)
    : m_Angle(0.0),
      m_TrapezoidFactor(0),
      m_DisplayOffset(0,0)
{
    m_CamExtents = CamExtents; 
    m_DistortionParams.push_back(0);
    m_DistortionParams.push_back(0);
    m_DisplayScale.x = DisplayExtents.x/CamExtents.x;
    m_DisplayScale.y = DisplayExtents.y/CamExtents.y;
    m_RescaleFactor = calc_rescale();
}

DeDistort::DeDistort(const DPoint &CamExtents,
            const std::vector<double>& DistortionParams,
            double Angle, double TrapezoidFactor,
            const DPoint& DisplayOffset, const DPoint& DisplayScale)
      : m_CamExtents(CamExtents),
        m_DistortionParams(DistortionParams),
        m_Angle(Angle),
        m_TrapezoidFactor(TrapezoidFactor),
        m_DisplayOffset(DisplayOffset),
        m_DisplayScale(DisplayScale)
{
    m_RescaleFactor = calc_rescale();
}

DeDistort::~DeDistort()
{
}

DRect DeDistort::getActiveBlobArea(const DPoint& DisplayExtents)
{
    DRect ActiveRect;
    ActiveRect.tl = transformScreenToBlob(DPoint(0, 0));
    ActiveRect.br = transformScreenToBlob(DPoint(DisplayExtents.x, DisplayExtents.y));
    if (ActiveRect.height() < 1) {
        double temp = ActiveRect.tl.y;
        ActiveRect.tl.y = ActiveRect.br.y;
        ActiveRect.br.y = temp;
    } 
    return ActiveRect;
}

void DeDistort::load(const DPoint& CamExtents, xmlNodePtr pParentNode)
{
    m_CamExtents = CamExtents;
    xmlNodePtr curXmlChild = pParentNode->xmlChildrenNode;
    while (curXmlChild) {
        const char * pNodeName = (const char *)curXmlChild->name;
        if (!strcmp(pNodeName, "distortionparams")) {
            m_DistortionParams.clear();
            m_DistortionParams.push_back(getRequiredDoubleAttr(curXmlChild, "p2"));
            m_DistortionParams.push_back(getRequiredDoubleAttr(curXmlChild, "p3"));
        } else if (!strcmp(pNodeName, "trapezoid")) {
            m_TrapezoidFactor = getRequiredDoubleAttr(curXmlChild, "value");
        } else if (!strcmp(pNodeName, "angle")) {
            m_Angle = getRequiredDoubleAttr(curXmlChild, "value");
        } else if (!strcmp(pNodeName, "displaydisplacement")) {
            m_DisplayOffset.x = getRequiredDoubleAttr(curXmlChild, "x");
            m_DisplayOffset.y = getRequiredDoubleAttr(curXmlChild, "y");
        } else if (!strcmp(pNodeName, "displayscale")) {
            m_DisplayScale.x = getRequiredDoubleAttr(curXmlChild, "x");
            m_DisplayScale.y = getRequiredDoubleAttr(curXmlChild, "y");
        }
        curXmlChild = curXmlChild->next;
    }
    m_RescaleFactor = calc_rescale();
}

void DeDistort::save(xmlTextWriterPtr writer)
{
    int rc;
    rc = xmlTextWriterStartElement(writer, BAD_CAST "transform");
    rc = xmlTextWriterStartElement(writer, BAD_CAST "distortionparams");
    writeAttribute(writer, "p2", m_DistortionParams[0]);
    writeAttribute(writer, "p3", m_DistortionParams[1]);
    rc = xmlTextWriterEndElement(writer);
    writeSimpleXMLNode(writer, "trapezoid", m_TrapezoidFactor);
    writeSimpleXMLNode(writer, "angle", m_Angle);
    writePoint(writer, "displaydisplacement", m_DisplayOffset);
    writePoint(writer, "displayscale", m_DisplayScale);
    rc = xmlTextWriterEndElement(writer);
}

bool DeDistort::operator ==(const DeDistort& other) const
{
    return (m_CamExtents == other.m_CamExtents &&
        m_DistortionParams == other.m_DistortionParams &&
        m_Angle == other.m_Angle &&
        m_TrapezoidFactor == other.m_TrapezoidFactor &&
        m_DisplayOffset == other.m_DisplayOffset &&
        m_DisplayScale == other.m_DisplayScale &&
        m_RescaleFactor == other.m_RescaleFactor);
}
        
void DeDistort::dump() const
{
    cerr << "  Transform:" << endl;
    cerr << "    CamExtents: " << m_CamExtents << endl;
    cerr << "    DistortionParams: " << m_DistortionParams[0] << ", " 
            << m_DistortionParams[1] << endl;
    cerr << "    Trapezoid: " << m_TrapezoidFactor << endl;
    cerr << "    Angle: " << m_Angle << endl;
    cerr << "    DisplayOffset: " << m_DisplayOffset << endl;
    cerr << "    DisplayScale: " << m_DisplayScale << endl;
}

DPoint DeDistort::transformScreenToBlob(const DPoint &pt)
{
    // scale to blob image resolution and translate 0,0 to upper left corner.
    DPoint DestPt = pt-m_DisplayOffset;
    DestPt = DPoint(DestPt.x/m_DisplayScale.x, DestPt.y/m_DisplayScale.y);
    return DestPt;
}

DPoint DeDistort::inverse_transform_point(const DPoint &pt)
{
    DPoint DestPt = pt-m_CamExtents/2;
    DestPt = DPoint(2*DestPt.x/m_CamExtents.x, 2*DestPt.y/m_CamExtents.y);
    DestPt = inv_trapezoid(m_TrapezoidFactor, DestPt);
    DestPt = rotate(DestPt, -m_Angle);
    DestPt *= m_RescaleFactor;
    DestPt = inverse_undistort(m_DistortionParams, DestPt);
    DestPt = DPoint(DestPt.x*m_CamExtents.x/2, DestPt.y*m_CamExtents.y/2);
    DestPt += m_CamExtents/2;
    return DestPt;
}

DPoint DeDistort::transformBlobToScreen(const DPoint &pt)
{
    DPoint DestPt = DPoint(m_DisplayScale.x*pt.x, m_DisplayScale.y*pt.y);
    DestPt += m_DisplayOffset;
    return DestPt;
}

DPoint DeDistort::transform_point(const DPoint &pt)
{
    DPoint DestPt = pt-m_CamExtents/2;
    DestPt = DPoint(2*DestPt.x/m_CamExtents.x, 2*DestPt.y/m_CamExtents.y);
    DestPt = undistort(m_DistortionParams, DestPt);
    DestPt /= m_RescaleFactor;
    DestPt = rotate(DestPt, m_Angle);
    DestPt = trapezoid(m_TrapezoidFactor, DestPt);
    DestPt = DPoint(DestPt.x*m_CamExtents.x/2, DestPt.y*m_CamExtents.y/2);
    DestPt += m_CamExtents/2;
    return DestPt;
}

DPoint DeDistort::inv_trapezoid(const double trapezoid_factor, const DPoint &pt)
{
    //stretch x coord
    double yn = pt.y;
    return DPoint(pt.x/(1+yn*trapezoid_factor), pt.y);
}

DPoint DeDistort::trapezoid(const double trapezoid_factor, const DPoint &pt)
{
    //stretch x coord
    double yn = pt.y;
    return DPoint(pt.x * (1 + yn*trapezoid_factor), pt.y);
}

double distort_map(const std::vector<double> &params, double r) 
{
    double S = 0;
    int counter = 3;
    std::vector<double>::const_iterator v;
    for(v=params.begin(); v!=params.end(); ++v){
        S += (*v) * pow(r, counter);
        ++counter;
    }
    return r+S;
}

double DeDistort::calc_rescale()
{
    //make sure that the undistort transformation stays within the normalized box
    double scale = distort_map(m_DistortionParams, sqrt(2.0));
    return scale/sqrt(2.0);
}

double inv_distort_map(const std::vector<double> &params, double r) 
{
        double r1,r2,r3,f1,f2;
        r1 = r;
        r2 = r+.001;
        f1 = distort_map(params, r1)-r;
        f2 = distort_map(params, r2)-r;
        while (fabs(f2) > 0.0001) {
            r3 = (r1*f2-r2*f1)/(f2-f1);
            r1 = r2;
            r2 = r3;
            f1 = f2;
            f2 = distort_map(params, r2)-r;
        }
        return r2;
}

#define EPSILON 0.00001
DPoint DeDistort::inverse_undistort(const std::vector<double> &params, const DPoint &pt)
{
    if ( params.empty() ) {
        return pt;
    }
    DPoint pt_norm = pt;
    double r_d = sqrt(pt_norm.x*pt_norm.x + pt_norm.y*pt_norm.y);
    double S;
    if (r_d < EPSILON) {
        S=0;
    } else {
        S = inv_distort_map(params, r_d)/r_d;
    }
    DPoint result = pt_norm*(S);
    return result;
}

DPoint DeDistort::undistort(const std::vector<double> &params, const DPoint &pt) 
{
    std::vector<double>::const_iterator v = params.begin();
    if ( v == params.end() ) {
        return pt;
    }
    DPoint pt_norm = pt;
    double r_d = sqrt(pt_norm.x*pt_norm.x + pt_norm.y*pt_norm.y);
    double S;
    if (r_d < EPSILON) {
        S=0;
    } else {
        S = distort_map(params, r_d)/r_d;
    }
    
    DPoint result = pt_norm*(S);
    return result;
}
    
}
