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

#ifndef _DirEntry_H_
#define _DirEntry_H_

#include "../api.h"
#include <boost/shared_ptr.hpp>

#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#endif
#include <string>

namespace avg {
    
class AVG_API DirEntry {
public:
#ifdef _WIN32
    DirEntry(std::string sDirName, const _finddata_t& findData);
#else
    DirEntry(std::string sDirName, dirent * pEntry);
#endif
    virtual ~DirEntry();

    std::string getName();
    void remove();

private:
    std::string m_sDirName;

#ifdef _WIN32
    _finddata_t m_FindData;
#else
    dirent * m_pEntry;
#endif

};

typedef boost::shared_ptr<DirEntry> DirEntryPtr;

}

#endif 



