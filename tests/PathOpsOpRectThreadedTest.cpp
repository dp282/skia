/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "PathOpsExtendedTest.h"
#include "PathOpsThreadedCommon.h"
#include "SkString.h"

// four rects, of four sizes
// for 3 smaller sizes, tall, wide
    // top upper mid lower bottom aligned (3 bits, 5 values)
    // same with x (3 bits, 5 values)
// not included, square, tall, wide (2 bits)
// cw or ccw (1 bit)

static int loopNo = 6;

static void testPathOpsRectsMain(PathOpsThreadState* data)
{
    SkASSERT(data);
    PathOpsThreadState& state = *data;
    SkString pathStr;
    for (int a = 0 ; a < 6; ++a) {
        for (int b = a + 1 ; b < 7; ++b) {
            for (int c = 0 ; c < 6; ++c) {
                for (int d = c + 1 ; d < 7; ++d) {
                    for (int e = SkPath::kWinding_FillType ; e <= SkPath::kEvenOdd_FillType; ++e) {
    for (int f = SkPath::kWinding_FillType ; f <= SkPath::kEvenOdd_FillType; ++f)   {
        SkPath pathA, pathB;
        pathA.setFillType((SkPath::FillType) e);
        pathA.addRect(SkIntToScalar(state.fA), SkIntToScalar(state.fA), SkIntToScalar(state.fB),
                SkIntToScalar(state.fB), SkPath::kCW_Direction);
        pathA.addRect(SkIntToScalar(state.fC), SkIntToScalar(state.fC), SkIntToScalar(state.fD),
                SkIntToScalar(state.fD), SkPath::kCW_Direction);
        pathA.close();
        pathB.setFillType((SkPath::FillType) f);
        pathB.addRect(SkIntToScalar(a), SkIntToScalar(a), SkIntToScalar(b),
                SkIntToScalar(b), SkPath::kCW_Direction);
        pathB.addRect(SkIntToScalar(c), SkIntToScalar(c), SkIntToScalar(d),
                SkIntToScalar(d), SkPath::kCW_Direction);
        pathB.close();
        for (int op = 0 ; op <= kXOR_SkPathOp; ++op)    {
            if (state.fReporter->verbose()) {
                pathStr.printf(
                        "static void rects%d(skiatest::Reporter* reporter,"
                        "const char* filename) {\n", loopNo);
                pathStr.appendf("    SkPath path, pathB;");
                pathStr.appendf("    path.setFillType(SkPath::k%s_FillType);\n",
                        e == SkPath::kWinding_FillType ? "Winding" : e == SkPath::kEvenOdd_FillType
                        ? "EvenOdd" : "?UNDEFINED");
                pathStr.appendf("    path.addRect(%d, %d, %d, %d,"
                        " SkPath::kCW_Direction);\n", state.fA, state.fA, state.fB, state.fB);
                pathStr.appendf("    path.addRect(%d, %d, %d, %d,"
                        " SkPath::kCW_Direction);\n", state.fC, state.fC, state.fD, state.fD);
                pathStr.appendf("    pathB.setFillType(SkPath::k%s_FillType);\n",
                        f == SkPath::kWinding_FillType ? "Winding" : f == SkPath::kEvenOdd_FillType
                        ? "EvenOdd" : "?UNDEFINED");
                pathStr.appendf("    pathB.addRect(%d, %d, %d, %d,"
                        " SkPath::kCW_Direction);\n", a, a, b, b);
                pathStr.appendf("    pathB.addRect(%d, %d, %d, %d,"
                        " SkPath::kCW_Direction);\n", c, c, d, d);
                pathStr.appendf("    testPathOp(reporter, path, pathB, %s, filename);\n",
                        SkPathOpsDebug::OpStr((SkPathOp) op));
                pathStr.appendf("}\n\n");
                outputProgress(state.fPathStr, pathStr.c_str(), (SkPathOp) op);
            }
            if (!testPathOp(state.fReporter, pathA, pathB, (SkPathOp) op, "rects")) {
                if (state.fReporter->verbose()) {
                    ++loopNo;
                    goto skipToNext;
                }
            }
        }
    }
                    }
skipToNext: ;
                }
            }
        }
    }
}

DEF_TEST(PathOpsRectsThreaded, reporter) {
    initializeTests(reporter, "testOp");
    PathOpsThreadedTestRunner testRunner(reporter);
    for (int a = 0; a < 6; ++a) {  // outermost
        for (int b = a + 1; b < 7; ++b) {
            for (int c = 0 ; c < 6; ++c) {
                for (int d = c + 1; d < 7; ++d) {
                    *testRunner.fRunnables.append() = new PathOpsThreadedRunnable(
                            &testPathOpsRectsMain, a, b, c, d, &testRunner);
                }
            }
            if (!reporter->allowExtendedTest()) goto finish;
       }
    }
finish:
    testRunner.render();
}
