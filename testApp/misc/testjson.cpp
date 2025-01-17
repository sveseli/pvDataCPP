/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <testMain.h>

#include <pv/pvdVersion.h>

#include <pv/json.h>
#include <pv/bitSet.h>
#include <pv/valueBuilder.h>
#include <pv/pvUnitTest.h>

namespace pvd = epics::pvData;

namespace {

void testparseany()
{
    testDiag("testparseany()");
    std::istringstream strm("{\"hello\":42, \"one\":{\"two\":\"test\"}}");

    pvd::PVStructurePtr val(pvd::parseJSON(strm));

    std::cout<<val<<"\n";

    testFieldEqual<pvd::PVLong>(val, "hello", 42);
    testFieldEqual<pvd::PVString>(val, "one.two", "test");
}

void testparseanyarray()
{
    testDiag("testparseanyarray()");
    std::istringstream strm("{\"hello\":[42, 43]}");

    pvd::PVStructurePtr val(pvd::parseJSON(strm));

    std::cout<<val<<"\n";

    pvd::PVLongArray::svector arr(2);
    arr[0] = 42;
    arr[1] = 43;
    pvd::PVLongArray::const_svector sarr(pvd::freeze(arr));

    testFieldEqual<pvd::PVLongArray>(val, "hello", sarr);
}

void testparsebare()
{
    testDiag("testparsebare()");
    {
        std::istringstream strm("5");

        pvd::PVIntPtr fld(pvd::getPVDataCreate()->createPVScalar<pvd::PVInt>());
        pvd::BitSet bits;

        pvd::parseJSON(strm, fld, &bits);

        testOk1(fld->get()==5);
        testEqual(bits, pvd::BitSet().set(0));
    }
    {
        std::istringstream strm("\"5\"");

        pvd::PVIntPtr fld(pvd::getPVDataCreate()->createPVScalar<pvd::PVInt>());

        pvd::parseJSON(strm, fld);

        testOk1(fld->get()==5);
    }
    {
        std::istringstream strm("\"hello\"");

        pvd::PVStringPtr fld(pvd::getPVDataCreate()->createPVScalar<pvd::PVString>());

        pvd::parseJSON(strm, fld);

        testOk1(fld->get()=="hello");
    }
}

void testparseanyjunk()
{
    testDiag("testparseanyjunk()");

    // things we can't store
    {
        testThrows(std::runtime_error, std::istringstream strm("4"); std::cout<<pvd::parseJSON(strm); );
    }
    {
        testThrows(std::runtime_error, std::istringstream strm("[]"); std::cout<<pvd::parseJSON(strm); );
    }
    // junk found after parsing completes
    {
        testThrows(std::runtime_error, std::istringstream strm("{} x"); std::cout<<pvd::parseJSON(strm) );
    }
#ifndef EPICS_YAJL_VERSION
    {
        testThrows(std::runtime_error, std::istringstream strm("{} /* y */"); std::cout<<pvd::parseJSON(strm) );
    }
    {
        testThrows(std::runtime_error, std::istringstream strm("{} /* y *"); std::cout<<pvd::parseJSON(strm) );
    }
#else
    testSkip(2, "yajl >= 2.1.0 handles trailing comments for us");
#endif
    {
        testThrows(std::runtime_error, std::istringstream strm("{}\n\n{}"); std::cout<<pvd::parseJSON(strm) );
    }
}


const char bigtest[] =
        "{\"scalar\":42,\n"
        " \"ivec\":[1,2,3], /*comment*/ \n"
        " \"svec\":[\"one\", \"two\"],\n"
        " \"sub\":{\"x\":{\"y\":43}},\n"
        " \"sarr\":[\n"
        "  /* another comment */\n"
        "   {\"a\":5, \"b\":6}\n"
        "  ,{\"a\":7, \"b\":8}\n"
        "  ,{\"a\":9, \"b\":10}\n"
        "  ],\n"
        " \"any\": \"4.2\",\n"
        " \"almost\": \"hello\"\n"
        "}";
// intentionally not setting "extra"

pvd::StructureConstPtr bigtype(pvd::getFieldCreate()->createFieldBuilder()
                            ->add("scalar", pvd::pvInt)
                            ->addArray("ivec", pvd::pvLong)
                            ->addArray("svec", pvd::pvString)
                            ->addNestedStructure("sub")
                                ->addNestedStructure("x")
                                    ->add("y", pvd::pvInt)
                                ->endNested()
                            ->endNested()
                            ->add("extra", pvd::pvInt)
                            ->addNestedStructureArray("sarr")
                                ->add("a", pvd::pvInt)
                                ->add("b", pvd::pvInt)
                            ->endNested()
                            ->add("any", pvd::getFieldCreate()->createVariantUnion())
                            ->addNestedUnion("almost")
                                ->add("one", pvd::pvInt)
                                ->add("two", pvd::pvString)
                            ->endNested()
                            ->createStructure()
                            );

void testInto()
{
    testDiag("testInto()");

    pvd::PVStructurePtr val(pvd::getPVDataCreate()->createPVStructure(bigtype));

    std::istringstream strm(bigtest);
    pvd::BitSet assigned;

    std::cout<<val;

    pvd::parseJSON(strm, val, &assigned);

    testEqual(assigned, pvd::BitSet()
              .set(val->getSubField("scalar")->getFieldOffset())
              .set(val->getSubField("ivec")->getFieldOffset())
              .set(val->getSubField("svec")->getFieldOffset())
              .set(val->getSubField("sub.x.y")->getFieldOffset())
              // "extra" not set
              .set(val->getSubField("sarr")->getFieldOffset())
              .set(val->getSubField("any")->getFieldOffset())
              .set(val->getSubField("almost")->getFieldOffset()));

    std::cout<<val;

    testFieldEqual<pvd::PVInt>(val, "scalar", 42);
    testFieldEqual<pvd::PVInt>(val, "sub.x.y", 43);
    {
        pvd::PVLongArray::svector expect(3);
        expect[0] = 1;
        expect[1] = 2;
        expect[2] = 3;
        testFieldEqual<pvd::PVLongArray>(val, "ivec", pvd::freeze(expect));
    }
    {
        pvd::PVStringArray::svector expect(2);
        expect[0] = "one";
        expect[1] = "two";
        testFieldEqual<pvd::PVStringArray>(val, "svec", pvd::freeze(expect));
    }
    {
        pvd::PVStructureArrayPtr sarr(val->getSubFieldT<pvd::PVStructureArray>("sarr"));
        pvd::PVStructureArray::const_svector elems(sarr->view());
        testEqual(elems.size(), 3u);
        if(elems.size()<3)
            testAbort("Missing elements");
        testFieldEqual<pvd::PVInt>(elems[0], "a", 5);
        testFieldEqual<pvd::PVInt>(elems[0], "b", 6);
        testFieldEqual<pvd::PVInt>(elems[1], "a", 7);
        testFieldEqual<pvd::PVInt>(elems[1], "b", 8);
        testFieldEqual<pvd::PVInt>(elems[2], "a", 9);
        testFieldEqual<pvd::PVInt>(elems[2], "b", 10);
    }

    testFieldEqual<pvd::PVString>(val, "any", "4.2");
    testFieldEqual<pvd::PVString>(val, "almost", "hello");
}

void testroundtrip()
{
    testDiag("testroundtrip()");

    testDiag("Parse expected");
    pvd::PVStructurePtr val(pvd::getPVDataCreate()->createPVStructure(bigtype));
    {
        std::istringstream strm(bigtest);
        pvd::parseJSON(strm, val);
    }
    std::cout<<val;

    testDiag("re-print parsed value");
    std::string round1;
    {
        pvd::JSONPrintOptions opts;
        opts.ignoreUnprintable = true;
        opts.multiLine = true;

        std::ostringstream strm;
        pvd::printJSON(strm, val, opts);

        round1 = strm.str();
    }
    std::cout<<round1<<"\n";

    testDiag("re-parse re-printed value");
    pvd::PVStructurePtr val2(pvd::getPVDataCreate()->createPVStructure(bigtype));
    {
        std::istringstream strm(round1);
        pvd::parseJSON(strm, val2);
    }

    testEqual(*val, *val2);

    testDiag("print value");
    std::string round2;
    {
        pvd::JSONPrintOptions opts;
        opts.ignoreUnprintable = true;
        opts.multiLine = false;

        std::ostringstream strm;
        pvd::printJSON(strm, val, opts);

        round2 = strm.str();
    }

    testEqual(round2, "{\"scalar\": 42,"
                       "\"ivec\": [1,2,3],"
                       "\"svec\": [\"one\",\"two\"],"
                       "\"sub\": {"
                         "\"x\": {"
                          "\"y\": 43"
                       "}},"
                       "\"extra\": 0,"
                       "\"sarr\": [{\"a\": 5,\"b\": 6},"
                                  "{\"a\": 7,\"b\": 8},"
                                  "{\"a\": 9,\"b\": 10}],"
                      "\"any\": \"4.2\","
                      "\"almost\": \"hello\""
                      "}");
}

pvd::PVStructurePtr createTestStructure(const std::string& fieldName, const std::string& fieldValue)
{
    static pvd::PVDataCreatePtr pvDataCreate = pvd::getPVDataCreate();
    pvd::PVScalarPtr f1 = pvDataCreate->createPVScalar(pvd::pvString);
    pvd::PVFieldPtrArray pvFields;
    pvd::StringArray fieldNames;
    pvFields.reserve(1);
    fieldNames.reserve(1);
    fieldNames.push_back(fieldName);
    pvFields.push_back(f1);
    pvd::PVStructurePtr pvStructurePtr = pvDataCreate->createPVStructure(fieldNames, pvFields);
    pvStructurePtr->getSubField<pvd::PVString>(fieldName)->put(fieldValue);
    return pvStructurePtr;
}

void testPrintJson(const std::string& fieldName, const std::string& fieldValue, const std::string& expectedOutput)
{
    pvd::PVStructurePtr pvStructurePtr = createTestStructure(fieldName, fieldValue);

    pvd::JSONPrintOptions opts;
    opts.ignoreUnprintable = true;
    opts.multiLine = false;
    pvd::BitSetPtr bitSet(new pvd::BitSet(pvStructurePtr->getStructure()->getNumberFields()));
    bitSet->set(0);
    std::ostringstream strm;
    pvd::printJSON(strm,*pvStructurePtr,*bitSet,opts);
    std::string result = strm.str();
    bool compare = (result == expectedOutput);
    std::cout << "testPrintJson: result=" << result << "; expected=" << expectedOutput <<   "; comparison=" << compare << std::endl;
    testOk1(compare == true);
}

void testPrintJson() 
{
    testPrintJson("foo", "bar", "{\"foo\": \"bar\"}");
    testPrintJson("foo", "\"bar\"", "{\"foo\": \"\\\"bar\\\"\"}");
    testPrintJson("foo", "\"long string with several \" characters\"", "{\"foo\": \"\\\"long string with several \\\" characters\\\"\"}");
    testPrintJson("foo", "\"long string with several \" and ' characters\"", "{\"foo\": \"\\\"long string with several \\\" and ' characters\\\"\"}");
}

} // namespace

MAIN(testjson)
{
    testPlan(33);
    try {
        testparseany();
        testparseanyarray();
        testparsebare();
        testparseanyjunk();
        testInto();
        testroundtrip();
        testPrintJson();
    }catch(std::exception& e){
        testAbort("Unexpected exception: %s", e.what());
    }
    return testDone();
}
