/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <string>

#include <pv/pvUnitTest.h>
#include <testMain.h>

#include <pv/pvData.h>
#include <pv/json.h>
#include <pv/bitSet.h>

using namespace epics::pvData;

static PVDataCreatePtr pvDataCreate = getPVDataCreate();

static PVStructurePtr createTestStructure(const std::string& fieldName, const std::string& fieldValue)
{
    PVScalarPtr f1 = pvDataCreate->createPVScalar(pvString);
    PVFieldPtrArray pvFields;
    StringArray fieldNames;
    pvFields.reserve(1);
    fieldNames.reserve(1);
    fieldNames.push_back(fieldName);
    pvFields.push_back(f1);
    PVStructurePtr pvStructurePtr = pvDataCreate->createPVStructure(fieldNames, pvFields);
    pvStructurePtr->getSubField<PVString>(fieldName)->put(fieldValue);
    return pvStructurePtr;
}

static void testToJson(const std::string& fieldName, const std::string& fieldValue, const std::string& expectedOutput)
{
    PVStructurePtr pvStructurePtr = createTestStructure(fieldName, fieldValue);

    JSONPrintOptions opts;
    opts.ignoreUnprintable = true;
    opts.multiLine = false;
    BitSetPtr bitSet(new BitSet(pvStructurePtr->getStructure()->getNumberFields()));
    bitSet->set(0);
    std::ostringstream strm;
    printJSON(strm,*pvStructurePtr,*bitSet,opts);
    std::string result = strm.str();
    bool compare = (result == expectedOutput);
    std::cout << "testToJson: result=" << result << "; expected=" << expectedOutput <<   "; comparison=" << compare << std::endl;
    testOk1(compare == true);
}


MAIN(testJson)
{
    testPlan(4);
    try {
        testToJson("foo", "bar", "{\"foo\": \"bar\"}");
        testToJson("foo", "\"bar\"", "{\"foo\": \"\\\"bar\\\"\"}");
        testToJson("foo", "\"long string with several \" characters\"", "{\"foo\": \"\\\"long string with several \\\" characters\\\"\"}");
        testToJson("foo", "\"long string with several \" and ' characters\"", "{\"foo\": \"\\\"long string with several \\\" and ' characters\\\"\"}");
    }
    catch(std::exception& e){
        PRINT_EXCEPTION(e);
        testAbort("Unhandled Exception: %s", e.what());
    }
    return testDone();
}
