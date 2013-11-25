/* testPVdata.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/* Author:  Marty Kraimer Date: 2010.11 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstring>
#include <cstdio>

#include <epicsUnitTest.h>
#include <testMain.h>

#include <pv/requester.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/convert.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/alarm.h>
#include <pv/control.h>
#include <pv/display.h>
#include <pv/timeStamp.h>
#include <pv/pvAlarm.h>
#include <pv/pvControl.h>
#include <pv/pvDisplay.h>
#include <pv/pvEnumerated.h>
#include <pv/pvTimeStamp.h>

using namespace epics::pvData;

static bool debug = false;

static FieldCreatePtr fieldCreate;
static PVDataCreatePtr pvDataCreate;
static StandardFieldPtr standardField;
static StandardPVFieldPtr standardPVField;
static ConvertPtr convert;
static String builder;
static String alarmTimeStamp("alarm,timeStamp");
static String allProperties("alarm,timeStamp,display,control");

static PVStructurePtr doubleRecord;
static PVStructurePtr enumeratedRecord;

static void createRecords()
{
    doubleRecord = standardPVField->scalar(pvDouble,allProperties);
    if(debug) {
        builder.clear();
        doubleRecord->toString(&builder);
        printf("doubleRecord\n%s\n",builder.c_str());
    }
    StringArray choices;
    choices.reserve(4);
    choices.push_back("1");
    choices.push_back("2");
    choices.push_back("3");
    choices.push_back("4");
    enumeratedRecord = standardPVField->enumerated(choices,alarmTimeStamp);
    if(debug) {
        builder.clear();
        enumeratedRecord->toString(&builder);
        printf("enumeratedRecord\n%s\n",builder.c_str());
    }
}

static void printRecords()
{
    printf("doubleRecord\n");
    builder.clear();
    doubleRecord->toString(&builder);
    printf("%s\n",builder.c_str());
    printf("enumeratedRecord\n");
    builder.clear();
    enumeratedRecord->toString(&builder);
    printf("%s\n",builder.c_str());
}

static void testAlarm()
{
    if(debug) printf("testAlarm\n");
    Alarm alarm;
    PVAlarm pvAlarm; 
    bool result;
    PVFieldPtr pvField = doubleRecord->getSubField(String("alarm"));
    if(pvField.get()==NULL) {
        printf("testAlarm ERROR did not find field alarm\n");
        return;
    }
    result = pvAlarm.attach(pvField);
    testOk1(result);
    Alarm al;
    al.setMessage(String("testMessage"));
    al.setSeverity(majorAlarm);
    al.setStatus(clientStatus);
    result = pvAlarm.set(al);
    testOk1(result);
    pvAlarm.get(alarm);
    testOk1(al.getMessage().compare(alarm.getMessage())==0);
    testOk1(al.getSeverity()==alarm.getSeverity());
    testOk1(al.getStatus()==alarm.getStatus());
    String message = alarm.getMessage();
    String severity = (*AlarmSeverityFunc::getSeverityNames())[alarm.getSeverity()];
    String status = (*AlarmStatusFunc::getStatusNames())[alarm.getStatus()];
    if(debug) {
        printf(" message %s severity %s status %s\n",
            message.c_str(),severity.c_str(),status.c_str());
    }
    printf("testAlarm PASSED\n");
}

static void testTimeStamp()
{
    if(debug) printf("testTimeStamp\n");
    TimeStamp timeStamp;
    PVTimeStamp pvTimeStamp; 
    bool result;
    PVFieldPtr pvField = doubleRecord->getSubField(String("timeStamp"));
    if(pvField.get()==NULL) {
        printf("testTimeStamp ERROR did not find field timeStamp\n");
        return;
    }
    result = pvTimeStamp.attach(pvField);
    testOk1(result);
    TimeStamp ts;
    ts.getCurrent();
    ts.setUserTag(32);
    result = pvTimeStamp.set(ts);
    testOk1(result);
    pvTimeStamp.get(timeStamp);
    testOk1(ts.getSecondsPastEpoch()==timeStamp.getSecondsPastEpoch());
    testOk1(ts.getNanoSeconds()==timeStamp.getNanoSeconds());
    testOk1(ts.getUserTag()==timeStamp.getUserTag());
    time_t tt;
    timeStamp.toTime_t(tt);
    struct tm ctm;
    memcpy(&ctm,localtime(&tt),sizeof(struct tm));
    if(debug) {
        printf(
            "%4.4d.%2.2d.%2.2d %2.2d:%2.2d:%2.2d %d nanoSeconds isDst %s userTag %d\n",
            ctm.tm_year+1900,ctm.tm_mon + 1,ctm.tm_mday,
            ctm.tm_hour,ctm.tm_min,ctm.tm_sec,
            timeStamp.getNanoSeconds(),
            (ctm.tm_isdst==0) ? "false" : "true",
            timeStamp.getUserTag());
    }
    timeStamp.put(0,0);
    pvTimeStamp.set(timeStamp);
    printf("testTimeStamp PASSED\n");
}

static void testControl()
{
    if(debug) printf("testControl\n");
    Control control;
    PVControl pvControl; 
    bool result;
    PVFieldPtr pvField = doubleRecord->getSubField(String("control"));
    if(pvField.get()==NULL) {
        printf("testControl ERROR did not find field control\n");
        return;
    }
    result = pvControl.attach(pvField);
    testOk1(result);
    Control cl;
    cl.setLow(1.0);
    cl.setHigh(10.0);
    result = pvControl.set(cl);
    testOk1(result);
    pvControl.get(control);
    testOk1(cl.getLow()==control.getLow());
    testOk1(cl.getHigh()==control.getHigh());
    double low = control.getLow();
    double high = control.getHigh();
    if(debug) printf(" low %f high %f\n",low,high);
    printf("testControl PASSED\n");
}

static void testDisplay()
{
    if(debug) printf("testDisplay\n");
    Display display;
    PVDisplay pvDisplay; 
    bool result;
    PVFieldPtr pvField = doubleRecord->getSubField(String("display"));
    if(pvField.get()==NULL) {
        printf("testDisplay ERROR did not find field display\n");
        return;
    }
    result = pvDisplay.attach(pvField);
    testOk1(result);
    Display dy;
    dy.setLow(-10.0);
    dy.setHigh(-1.0);
    dy.setDescription(String("testDescription"));
    dy.setFormat(String("%f10.0"));
    dy.setUnits(String("volts"));
    result = pvDisplay.set(dy);
    testOk1(result);
    pvDisplay.get(display);
    testOk1(dy.getLow()==display.getLow());
    testOk1(dy.getHigh()==display.getHigh());
    testOk1(dy.getDescription().compare(display.getDescription())==0);
    testOk1(dy.getFormat().compare(display.getFormat())==0);
    testOk1(dy.getUnits().compare(display.getUnits())==0);
    double low = display.getLow();
    double high = display.getHigh();
    if(debug) printf(" low %f high %f\n",low,high);
    printf("testDisplay PASSED\n");
}

static void testEnumerated()
{
    if(debug) printf("testEnumerated\n");
    PVEnumerated pvEnumerated; 
    bool result;
    PVFieldPtr pvField = enumeratedRecord->getSubField(String("value"));
    if(pvField.get()==NULL) {
        printf("testEnumerated ERROR did not find field enumerated\n");
        return;
    }
    result = pvEnumerated.attach(pvField);
    testOk1(result);
    int32 index = pvEnumerated.getIndex();
    String choice = pvEnumerated.getChoice();
    PVStringArray::const_svector choices = pvEnumerated.getChoices();
    int32 numChoices = pvEnumerated.getNumberChoices();
    if(debug) {
        printf("index %d choice %s choices",index,choice.c_str());
        for(int i=0; i<numChoices; i++ ) printf(" %s",choices[i].c_str());
        printf("\n");
    }
    pvEnumerated.setIndex(2);
    index = pvEnumerated.getIndex();
    choice = pvEnumerated.getChoice();
    if(debug) printf("index %d choice %s\n",index,choice.c_str());
    printf("testEnumerated PASSED\n");
}

MAIN(testProperty)
{
    testPlan(22);
    testDiag("Tests property");
    fieldCreate = getFieldCreate();
    pvDataCreate = getPVDataCreate();
    standardField = getStandardField();
    standardPVField = getStandardPVField();
    convert = getConvert();
    createRecords();
    testAlarm();
    testTimeStamp();
    testControl();
    testDisplay();
    testEnumerated();
    if(debug) printRecords();
    return testDone();;
}

