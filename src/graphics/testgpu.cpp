//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2008 Ulrich von Zadow
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

#include "GraphicsTest.h"
#include "GPUBrightnessFilter.h"
#include "GPUBlurFilter.h"
#include "GPUBandpassFilter.h"
#include "OGLImagingContext.h"
#include "IteratingGPUFilter.h"

#include "../base/TestSuite.h"
#include "../base/Exception.h"
#include "../base/Test.h"
#include "../base/StringHelper.h"

#include <math.h>
#include <iostream>

using namespace avg;
using namespace std;

class FBOTest: public GraphicsTest {
public:
    FBOTest()
        : GraphicsTest("FBOTest", 2)
    {
    }

    void runTests() 
    {
        runImageTests("i8-64x64", GL_UNSIGNED_BYTE, I8);
        runImageTests("rgb24-64x64", GL_UNSIGNED_BYTE);
        runImageTests("rgb24alpha-64x64", GL_UNSIGNED_BYTE);
        runImageTests("i8-64x64", GL_FLOAT, I8);
        runImageTests("rgb24-64x64", GL_FLOAT);
        runImageTests("rgb24alpha-64x64", GL_FLOAT);

        runPBOFloatbufTestI32F();
        runPBOFloatbufTestR32G32B32A32F();
        
        runPBOBitmapTestR32G32B32A32F();
        runPBOBitmapTestI32F();

        runPBOBitmapTestR32G32B32A32FextR8G8B8A8();
        runPBOBitmapTestI32FextI8();
    }

private:
    void runImageTests(const string& sFName, int precision, PixelFormat pf = R8G8B8X8)
    {
        BitmapPtr pBmp = loadTestBmp(sFName, pf);
        cerr << "    Testing " << sFName << " (" << pBmp->getPixelFormatString() << ")" 
                << endl;
        PBOImage pbo(pBmp->getSize(), pBmp->getPixelFormat(), pBmp->getPixelFormat(), 
                true, true);
        runPBOImageTest(pbo, pBmp, string("pbo_")+sFName);
    }

    void compareByteArrays(unsigned char *in, unsigned char *out, int n) {
        for (int i=0; i<n; i++) {
            if (in[i] != out[i]) {
                TEST_FAILED("compareByteArrays: " + toString((int)in[i]) + " (in) != "
                        + toString((int)out[i]) + " (out)")
            }
        }
    }
    void compareFloatArrays(float *in, float *out, int n) {
        for (int i=0; i<n; i++) {
            if (fabs(in[i]-out[i])>1e-5) {
                TEST_FAILED("compareFloatArrays: " + toString(in[i]) + " (in) != " 
                        + toString(out[i]) + " (out)")
            }
        }
    }

    void setAndCompareImage(PBOImage *pbo, float *in, int n) {
        pbo->setImage(in);
        BitmapPtr res = pbo->getImage();
        float *out = (float*)res->getPixels();
        compareFloatArrays(in, out, n);
    }

    void fillFloatArray(float *data, int n) {
        for (int i=0;i<n;i++) {
            data[i] = 0.01f * i;
        }
    }
    void fillByteArray(unsigned char *data, int n) {
        for (int i = 0; i<n; i++) {
            data[i] = i * 2;
        }
    }
    void runPBOFloatbufTest(PixelFormat pf) {
        assert(pf == I32F || pf == R32G32B32A32F);
        IntPoint size = IntPoint(11, 3);
        int numFloats = size.x*size.y*Bitmap::getBytesPerPixel(pf)/sizeof(float);
        float *pixels = new float[numFloats];
        fillFloatArray(pixels, numFloats);

        PBOImage pbo(size, pf, pf, false, true);
        setAndCompareImage(&pbo, pixels, numFloats);
        delete[] pixels;

    }
    void runPBOFloatbufTestI32F()
    {
        cerr << "    runPBOFloatbufTestI32F" << endl;
        runPBOFloatbufTest(I32F);
        return;
    }

    void runPBOFloatbufTestR32G32B32A32F()
    {
        cerr << "    runPBOFloatbufTestR32G32B32A32F" << endl;
        runPBOFloatbufTest(R32G32B32A32F);
        return;
    }
    void runPBOFloatBitmapTest(PixelFormat pf)
    {
        assert(pf == I32F || pf == R32G32B32A32F);
        IntPoint size = IntPoint(5,3);
        int numFloats = size.x*size.y*Bitmap::getBytesPerPixel(pf)/sizeof(float);
        float *pixels = new float [numFloats];
        fillFloatArray(pixels, numFloats);

        PBOImagePtr pPBO = PBOImagePtr(new PBOImage(size, pf, pf, true, true));
        BitmapPtr pBmp = BitmapPtr (new Bitmap(size, pf, (unsigned char*)(pixels),
            size.x*Bitmap::getBytesPerPixel(pf), false));
        pPBO->setImage(pBmp);
        BitmapPtr res = pPBO->getImage();
        compareFloatArrays(pixels,(float*)res->getPixels(), numFloats);

        delete[] pixels;
    }

    void runPBOBitmapTestI32F()
    {
        cerr << "    runPBOBitmapTestI32F" << endl;
        runPBOFloatBitmapTest(I32F);
        return;
    }
    void runPBOBitmapTestR32G32B32A32F()
    {
        cerr << "    runPBOBitmapTestR32G32B32A32F" << endl;
        runPBOFloatBitmapTest(R32G32B32A32F);
        return;
    }

    void runPBOBitmapTestIntFloatExtByte(PixelFormat intPF, PixelFormat extPF)
    {
        IntPoint size = IntPoint(3,5);
        unsigned char *pixels = new unsigned char [
            size.x*size.y*Bitmap::getBytesPerPixel(extPF)];
        fillByteArray(pixels, size.x*size.y*Bitmap::getBytesPerPixel(extPF));

        PBOImagePtr pPBO = PBOImagePtr(new PBOImage(size, intPF, extPF, true, true));
        BitmapPtr pBmp = BitmapPtr (new Bitmap(size, extPF, pixels,
            size.x*Bitmap::getBytesPerPixel(extPF), false));
        pPBO->setImage(pBmp);
        BitmapPtr res = pPBO->getImage();
        compareByteArrays(pixels, res->getPixels(),
            size.x*size.y*Bitmap::getBytesPerPixel(extPF));

        delete[] pixels;
    }

    void runPBOBitmapTestR32G32B32A32FextR8G8B8A8()
    {
        cerr << "    runPBOFloatbufTestR32G32B32A32FextR8G8B8A8" << endl;
        runPBOBitmapTestIntFloatExtByte(R32G32B32A32F, R8G8B8A8);
    }
    
    void runPBOBitmapTestI32FextI8()
    {
        cerr << "    runPBOBitmapTestI32FextI8" << endl;
        runPBOBitmapTestIntFloatExtByte(I32F, I8);
    }

    void runPBOImageTest(PBOImage& pbo, BitmapPtr pBmp, const string& sFName)
    {
        pbo.setImage(pBmp);
        BitmapPtr pNewBmp = pbo.getImage();
        testEqual(*pNewBmp, *pBmp, sFName);
    }

};

class BrightnessFilterTest: public GraphicsTest {
public:
    BrightnessFilterTest()
        : GraphicsTest("BrightnessFilterTest", 2)
    {
    }

    void runTests() 
    {
        runImageTests("rgb24-64x64");
        runImageTests("rgb24alpha-64x64");
    }

private:
    void runImageTests(const string& sFName)
    {
        cerr << "    Testing " << sFName << endl;
        BitmapPtr pBmp = loadTestBmp(sFName);
        BitmapPtr pDestBmp;
        pDestBmp = GPUBrightnessFilter(pBmp->getSize(), pBmp->getPixelFormat(), 1).apply(pBmp);
        testEqual(*pDestBmp, *pBmp, string("brightness_")+sFName, 0.2, 0.5);
    }
};

class BlurFilterTest: public GraphicsTest {
public:
    BlurFilterTest()
        : GraphicsTest("BlurFilterTest", 2)
    {
    }

    void runTests() 
    {
        BitmapPtr pBmp;
        BitmapPtr pDestBmp;
/*
        // This has the effect of printing out all the brightness differences for different
        // kernel sizes.
        pBmp = loadTestBmp("spike");
        for (double stddev = 0.5; stddev < 5; stddev += 0.25) {
            pDestBmp = GPUBlurFilter(pBmp->getSize(), pBmp->getPixelFormat(), stddev).apply(pBmp);
            testEqualBrightness(*pDestBmp, *pBmp, 1);
        }
*/
        cerr << "    Testing spike, stddev 0.5" << endl;
        pBmp = loadTestBmp("spike");
        pDestBmp = GPUBlurFilter(pBmp->getSize(), pBmp->getPixelFormat(), 0.5).apply(pBmp);
        testEqualBrightness(*pDestBmp, *pBmp, 0.0004);
        testEqual(*pDestBmp, "blur05_spike", B8G8R8X8, 0.01, 0.1);
        cerr << "    Testing spike, stddev 1" << endl;
        pDestBmp = GPUBlurFilter(pBmp->getSize(), pBmp->getPixelFormat(), 1).apply(pBmp);
//        testEqualBrightness(*pDestBmp, *pBmp, 5);
        testEqual(*pDestBmp, "blur1_spike", B8G8R8X8, 0.01, 0.1);
        cerr << "    Testing spike, stddev 3" << endl;
        pDestBmp = GPUBlurFilter(pBmp->getSize(), pBmp->getPixelFormat(), 3).apply(pBmp);
//        testEqualBrightness(*pDestBmp, *pBmp, 5);
        testEqual(*pDestBmp, "blur5_spike", B8G8R8X8, 0.01, 0.1);

        cerr << "    Testing flat, stddev 5" << endl;
        pBmp = loadTestBmp("flat");
        pDestBmp = GPUBlurFilter(pBmp->getSize(), pBmp->getPixelFormat(), 5).apply(pBmp);
        testEqualBrightness(*pDestBmp, *pBmp, 1);
        testEqual(*pDestBmp, *pBmp, "blur05_flat");

        runImageTests("rgb24-64x64");
        runImageTests("rgb24alpha-64x64");
    }

private:
    void runImageTests(const string& sFName)
    {
        cerr << "    Testing " << sFName << endl;
        BitmapPtr pBmp = loadTestBmp(sFName);
        BitmapPtr pDestBmp;
        pDestBmp = GPUBlurFilter(pBmp->getSize(), pBmp->getPixelFormat(), 10).apply(pBmp);
        testEqual(*pDestBmp, string("blur_")+sFName, pBmp->getPixelFormat(), 0.2, 0.5);
    }
};

class BandpassFilterTest: public GraphicsTest {
public:
    BandpassFilterTest()
        : GraphicsTest("BandpassFilterTest", 2)
    {
    }

    void runTests() 
    {
        runImageTests("spike", B8G8R8X8);
        runImageTests("i8-64x64", I8);
    }

private:
    void runImageTests(const string& sFName, PixelFormat pf)
    {
        cerr << "    Testing " << sFName << endl;
        BitmapPtr pBmp = loadTestBmp(sFName, pf);
        GPUBandpassFilter f(pBmp->getSize(), pf, 0.5, 1.5, 1, false);
        BitmapPtr pDestBmp = f.apply(pBmp);
        cerr << "        " << pDestBmp->getAvg() << endl;
        TEST(fabs(pDestBmp->getAvg() -128) < 0.06);
        testEqual(*pDestBmp, "bandpass_"+sFName, pf, 0.2, 0.5);
        TEST(pDestBmp->getPixelFormat() == pf);
    }
};


class IteratingGPUTestFilter: public IteratingGPUFilter 
{
public:
    IteratingGPUTestFilter(const IntPoint& size)
        : IteratingGPUFilter(size, 1)
    {
        if (!s_pShader) {
            initShader();
        }
    }

protected:
    void initShader()
    {
        string sCode =
            "#extension GL_ARB_texture_rectangle : enable\n" 
            "uniform sampler2DRect Texture;\n"
            "void main(void)\n"
            "{\n"
            "    gl_FragColor = texture2DRect(Texture,\n"
            "           vec2(gl_TexCoord[0].s, gl_TexCoord[0].t-10.0));\n"
            "}\n";
        s_pShader = OGLShaderPtr(new OGLShader(sCode));
    }

    void applyOnce(PBOImagePtr pSrc)
    {
        s_pShader->activate();
        s_pShader->setUniformIntParam("Texture", 0);
        pSrc->draw();
    }

private:
    OGLShaderPtr s_pShader;
};


class IteratingGPUFilterTest: public GraphicsTest {
public:
    IteratingGPUFilterTest()
        : GraphicsTest("IteratingGPUFilterTest", 2)
    {
    }

    void runTests() 
    {
        BitmapPtr pBmp = loadTestBmp("spike", B8G8R8A8);
        BitmapPtr pSrcBmp(new Bitmap(pBmp->getSize(), R32G32B32A32F));
        pSrcBmp->copyPixels(*pBmp); 
        IteratingGPUTestFilter f(pBmp->getSize());
        BitmapPtr pDestBmp = f.apply(pSrcBmp);
        BitmapPtr pByteDestBmp(new Bitmap(pBmp->getSize(), B8G8R8A8));
        pByteDestBmp->copyPixels(*pDestBmp);
        testEqual(*pByteDestBmp, "iteratingGPUFilter", B8G8R8A8);
    }
};


class GPUTestSuite: public TestSuite {
public:
    GPUTestSuite() 
        : TestSuite("GPUTestSuite")
    {
        addTest(TestPtr(new FBOTest));
        addTest(TestPtr(new BrightnessFilterTest));
        addTest(TestPtr(new BlurFilterTest));
        addTest(TestPtr(new BandpassFilterTest));
        addTest(TestPtr(new IteratingGPUFilterTest));
    }
};


int main(int nargs, char** args)
{
    bool bOK = true;
    try {
        OGLImagingContext context(IntPoint(64, 64));
        try {
            if (!queryOGLExtension("GL_ARB_fragment_shader")) {
                throw Exception(AVG_ERR_UNSUPPORTED, 
                        "Fragment shaders not supported on this Machine. ");
            }
            GPUTestSuite Suite;
            Suite.runTests();
            bOK = Suite.isOk();
        } catch (Exception& ex) {
            cerr << "Exception: " << ex.GetStr() << endl;
        }
    } catch (Exception& ex) {
        cerr << "Skipping GPU imaging test." << endl;
        cerr << "Reason: " << ex.GetStr() << endl;
        bOK = true;
    }

    if (bOK) {
        return 0;
    } else {
        return 1;
    }
}

