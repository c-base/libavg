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

#include "TrackerConfig.h"

#include "../base/XMLHelper.h"
#include "../base/Logger.h"

#include <libxml/parser.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlstring.h>

#include <sstream>

using namespace std;

namespace avg {

    template<class T>
    void writeAttribute(xmlTextWriterPtr writer, string sName, T Value)
    {
        int rc;
        stringstream strs;
        strs << Value;
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST sName.c_str(), 
                BAD_CAST strs.str().c_str());
    }

    template<class T>
    void writeSimpleXMLNode(xmlTextWriterPtr writer, string sName, T Value)
    {
        int rc;
        rc = xmlTextWriterStartElement(writer, BAD_CAST sName.c_str());
        writeAttribute(writer, "value", Value);
        rc = xmlTextWriterEndElement(writer);
    }
        
    void writeMinMaxXMLNode(xmlTextWriterPtr writer, string sName, double Val[2])
    {
        int rc;
        rc = xmlTextWriterStartElement(writer, BAD_CAST sName.c_str());
        writeAttribute(writer, "min", Val[0]);
        writeAttribute(writer, "max", Val[1]);
        rc = xmlTextWriterEndElement(writer);
    }

    void writeRect(xmlTextWriterPtr writer, string sName, IntRect& Val)
    {
        int rc;
        rc = xmlTextWriterStartElement(writer, BAD_CAST sName.c_str());
        writeAttribute(writer, "tlx", Val.tl.x);
        writeAttribute(writer, "tly", Val.tl.y);
        writeAttribute(writer, "brx", Val.br.x);
        writeAttribute(writer, "bry", Val.br.y);
        rc = xmlTextWriterEndElement(writer);

    }

    TrackerConfig::TrackerConfig()
        : m_K1(0),
          m_T(0),
          m_Brightness(128),
          m_Exposure(128),
          m_Gain(128),
          m_Shutter(128),
          m_Threshold(20),
          m_HistoryUpdateInterval(5),
          m_Similarity(31)
    {
          m_AreaBounds[0] = 80;
          m_AreaBounds[1] = 450;
          m_EccentricityBounds[0] = 1; 
          m_EccentricityBounds[1] = 3;
          load("TrackerConfig.xml");
    } 
    
    TrackerConfig::~TrackerConfig()
    {
    }

    void TrackerConfig::load(std::string sFilename)
    {
        xmlDocPtr doc;
        doc = xmlParseFile(sFilename.c_str());
        if (!doc) {
            AVG_TRACE(Logger::ERROR, "Could not open tracker config file " 
                    << sFilename << ".");
            exit(5);
        }
        xmlNodePtr pRoot = xmlDocGetRootElement(doc);
        xmlNodePtr curXmlChild = pRoot->xmlChildrenNode;
        while (curXmlChild) {
            const char * pNodeName = (const char *)curXmlChild->name;
            if (!strcmp(pNodeName, "brightness")) {
                m_Brightness = getRequiredIntAttr(curXmlChild, "value");
            } else if (!strcmp(pNodeName, "barrel")) {
                m_K1 = getRequiredDoubleAttr(curXmlChild, "value");
            } else if (!strcmp(pNodeName, "trapezoid")) {
                m_T = getRequiredDoubleAttr(curXmlChild, "value");
            } else if (!strcmp(pNodeName, "ROI")) {
                m_ROI.tl.x = getRequiredIntAttr(curXmlChild, "tlx");
                m_ROI.tl.y = getRequiredIntAttr(curXmlChild, "tly");
                m_ROI.br.x = getRequiredIntAttr(curXmlChild, "brx");
                m_ROI.br.y = getRequiredIntAttr(curXmlChild, "bry");
            } else if (!strcmp(pNodeName, "exposure")) {
                m_Exposure = getRequiredIntAttr(curXmlChild, "value");
            } else if (!strcmp(pNodeName, "gamma")) {
                m_Gamma = getRequiredIntAttr(curXmlChild, "value");
            } else if (!strcmp(pNodeName, "gain")) {
                m_Gain = getRequiredIntAttr(curXmlChild, "value");
            } else if (!strcmp(pNodeName, "shutter")) {
                m_Shutter = getRequiredIntAttr(curXmlChild, "value");
            } else if (!strcmp(pNodeName, "threshold")) {
                m_Threshold = getRequiredIntAttr(curXmlChild, "value");
            } else if (!strcmp(pNodeName, "historyupdateinterval")) {
                m_HistoryUpdateInterval = getRequiredIntAttr(curXmlChild, "value");
            } else if (!strcmp(pNodeName, "similarity")) {
                m_Similarity = getRequiredDoubleAttr(curXmlChild, "value");
            } else if (!strcmp(pNodeName, "areabounds")) {
                m_AreaBounds[0] = getRequiredIntAttr(curXmlChild, "min");
                m_AreaBounds[1] = getRequiredIntAttr(curXmlChild, "max");
            } else if (!strcmp(pNodeName, "eccentricitybounds")) {
                m_EccentricityBounds[0] = getRequiredDoubleAttr(curXmlChild, "min");
                m_EccentricityBounds[1] = getRequiredDoubleAttr(curXmlChild, "max");
            } else if (!strcmp(pNodeName, "debug")) {
                m_bDebug = getRequiredIntAttr(curXmlChild, "value");
            }
            curXmlChild = curXmlChild->next;
        }

    }

    void TrackerConfig::save(std::string sFilename)
    {
        xmlDocPtr doc;
        int rc;
        stringstream ss;
        xmlTextWriterPtr writer = xmlNewTextWriterDoc(&doc, 0);
        rc = xmlTextWriterSetIndent(writer, 4);
        rc = xmlTextWriterStartDocument(writer, NULL, "utf-8", NULL);
        rc = xmlTextWriterStartElement(writer, BAD_CAST "trackerconfig");
        writeSimpleXMLNode(writer, "barrel", m_K1);
        writeSimpleXMLNode(writer, "trapezoid", m_T);
        writeRect(writer, "ROI", m_ROI);
        writeSimpleXMLNode(writer, "brightness", m_Brightness);
        writeSimpleXMLNode(writer, "exposure", m_Exposure);
        writeSimpleXMLNode(writer, "gamma", m_Gamma);
        writeSimpleXMLNode(writer, "gain", m_Gain);
        writeSimpleXMLNode(writer, "shutter", m_Shutter);
        writeSimpleXMLNode(writer, "threshold", m_Threshold);
        writeSimpleXMLNode(writer, "historyupdateinterval", m_HistoryUpdateInterval);
        writeSimpleXMLNode(writer, "similarity", m_Similarity);
        writeMinMaxXMLNode(writer, "areabounds", m_AreaBounds);
        writeMinMaxXMLNode(writer, "eccentricitybounds", m_EccentricityBounds);
        writeSimpleXMLNode(writer, "debug", m_bDebug);
        rc = xmlTextWriterEndElement(writer);
        rc = xmlTextWriterEndDocument(writer);
        xmlFreeTextWriter(writer);
        xmlSaveFileEnc(sFilename.c_str(), doc, "utf-8");
        xmlFreeDoc(doc);
    }

}
