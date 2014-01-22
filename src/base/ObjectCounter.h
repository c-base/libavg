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

#ifndef _ObjectCounter_H_
#define _ObjectCounter_H_

#include "../api.h"
#include <string>
#include <map>
#include <typeinfo>

namespace avg {

typedef std::map<const std::type_info *, int> TypeMap;

class AVG_API ObjectCounter {
public:
    static ObjectCounter* get();
    virtual ~ObjectCounter();

    void incRef(const std::type_info* pType);
    void decRef(const std::type_info* pType);

    int getCount(const std::type_info* pType);

    std::string demangle(std::string s);
    std::string dump();
    TypeMap getObjectCount();

private:
    ObjectCounter();
    static void deleteSingleton();

    TypeMap m_TypeMap;

    static ObjectCounter* s_pObjectCounter;
    static bool s_bDeleted;
    friend void deleteObjectCounter();
};

}
#endif 

