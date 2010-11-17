/* testPVStructureArray.cpp */
/* Author:  Marty Kraimer Date: 2010.10 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>

#include <epicsAssert.h>

#include "requester.h"
#include "pvIntrospect.h"
#include "pvData.h"
#include "standardField.h"
#include "standardPVField.h"
#include "showConstructDestruct.h"

using namespace epics::pvData;

static FieldCreate * fieldCreate = 0;
static PVDataCreate * pvDataCreate = 0;
static StandardField *standardField = 0;
static StandardPVField *standardPVField = 0;
static String buffer("");

StructureConstPtr getPowerSupplyStructure() {
    String properties("alarm");
    FieldConstPtr powerSupply[3];
    powerSupply[0] = standardField->scalar(
        String("voltage"),pvDouble,properties);
    powerSupply[1] = standardField->scalar(
        String("power"),pvDouble,properties);
    powerSupply[2] = standardField->scalar(
        String("current"),pvDouble,properties);
    StructureConstPtr structure = standardField->structure(
        String("powerSupply"),3,powerSupply);
    return structure;
}

void testPowerSupplyArray(FILE * fd) {
    PVStructure* powerSupplyArrayStruct = standardPVField->structureArray(
        0,"powerSupply",getPowerSupplyStructure(),String("alarm,timeStamp"));
    PVStructureArray * powerSupplyArray =
        powerSupplyArrayStruct->getStructureArrayField(String("value"));
    assert(powerSupplyArray!=0);
    PVStructure *structureArray[3];
    StructureConstPtr structure =
        powerSupplyArray->getStructureArray()->getStructure();
    structureArray[0] = pvDataCreate->createPVStructure(0,structure);
    structureArray[1] = pvDataCreate->createPVStructure(0,structure);
    structureArray[2] = pvDataCreate->createPVStructure(0,structure);
    powerSupplyArray->put(0,3,structureArray,0);
    buffer.clear();
    powerSupplyArrayStruct->toString(&buffer);
    fprintf(fd,"%s\n",buffer.c_str());
    delete powerSupplyArrayStruct;
}

int main(int argc,char *argv[])
{
    char *fileName = 0;
    if(argc>1) fileName = argv[1];
    FILE * fd = stdout;
    if(fileName!=0 && fileName[0]!=0) {
        fd = fopen(fileName,"w+");
    }
    fieldCreate = getFieldCreate();
    pvDataCreate = getPVDataCreate();
    standardField = getStandardField();
    standardPVField = getStandardPVField();
    testPowerSupplyArray(fd);
    getShowConstructDestruct()->constuctDestructTotals(fd);
    return(0);
}

