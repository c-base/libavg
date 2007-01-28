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

#include "../graphics/Rect.h"
#include "TrackerCalibrator.h"

#include "../base/TestSuite.h"
#include "../base/Exception.h"
#include "../base/Logger.h"

#include <stdio.h>
#include <stdlib.h>

using namespace avg;
using namespace std;

class CalibratorTest: public Test {
public:
    CalibratorTest()
        : Test("CalibratorTest", 2)
    {
    }

    void runTests() 
    {
        {
            TrackerCalibrator Calibrator(IntPoint(640, 480), 
                IntRect(0,0,640,480), IntPoint(640,480));
            bool bDone = false;
            while (!bDone) {
                IntPoint DisplayPoint(Calibrator.getDisplayPointX(), 
                        Calibrator.getDisplayPointY());
                Calibrator.setCamPoint(DisplayPoint.x, DisplayPoint.y);
                bDone = Calibrator.nextPoint();
            }
//             CoordTransformerPtr Trafo = Calibrator.makeTransformer();

            // TODO: test if Trafo is an identity transformer.
        }

        {
            TrackerCalibrator Calibrator(IntPoint(640, 480), 
                IntRect(0,0,640,480), IntPoint(640,480));
            bool bDone = false;
            while (!bDone) {
                IntPoint DisplayPoint(Calibrator.getDisplayPointX(), 
                        Calibrator.getDisplayPointY());
                Calibrator.setCamPoint(DisplayPoint.x+DisplayPoint.y, DisplayPoint.y);
                bDone = Calibrator.nextPoint();
            }
//             CoordTransformerPtr Trafo = Calibrator.makeTransformer();

            // TODO: test if Trafo is a trapezoid transformer.
        }
    }
   
};

class CalibratorTestSuite: public TestSuite {
public:
    CalibratorTestSuite() 
        : TestSuite("CalibratorTestSuite")
    {
        addTest(TestPtr(new CalibratorTest));
    }
};


int main(int nargs, char** args)
{
    CalibratorTestSuite Suite;
    Suite.runTests();
    bool bOK = Suite.isOk();

    if (bOK) {
        return 0;
    } else {
        return 1;
    }
}

