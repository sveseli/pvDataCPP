/*PVAuxInfo.cpp*/
#include <cstddef>
#include <cstdlib>
#include <string>
#include <cstdio>
#include "noDefaultMethods.h"
#include "pvData.h"
#include "convert.h"
#include "factory.h"
#include "lock.h"

namespace epics { namespace pvData {

static volatile int64 totalConstruct = 0;
static volatile int64 totalDestruct = 0;
static Mutex *globalMutex = 0;

class CDCallbackPVAuxInfo : public ConstructDestructCallback {
public:
    CDCallbackPVAuxInfo();
    virtual String getConstructName();
    virtual int64 getTotalConstruct();
    virtual int64 getTotalDestruct();
    virtual int64 getTotalReferenceCount();
private:
    String name;
};

CDCallbackPVAuxInfo::CDCallbackPVAuxInfo()
: name("pvAuxInfo")
{
    getShowConstructDestruct()->registerCallback(this);
}
String CDCallbackPVAuxInfo::getConstructName() {return name;}
int64 CDCallbackPVAuxInfo::getTotalConstruct()
{
    Lock xx(globalMutex);
    return totalConstruct;
}
int64 CDCallbackPVAuxInfo::getTotalDestruct()
{
    Lock xx(globalMutex);
    return totalDestruct;
}
int64 CDCallbackPVAuxInfo::getTotalReferenceCount()
{
    return 0;
}

static ConstructDestructCallback *pConstructDestructCallback;

static void init()
{
    static Mutex mutex = Mutex();
    Lock xx(&mutex);
    if(globalMutex==0) {
        globalMutex = new Mutex();
        pConstructDestructCallback = new CDCallbackPVAuxInfo();
    }
}

typedef std::map<String,PVScalar * >::const_iterator map_iterator;

class PVAuxInfoPvt {
public:
    PVAuxInfoPvt(PVField *pvField)
    :  pvField(pvField),
       theMap(std::map<String, PVScalar * >())
    {} 
    PVField *pvField;
    std::map<String, PVScalar * > theMap;
};

PVAuxInfo::PVAuxInfo(PVField *pvField)
: pImpl(new PVAuxInfoPvt(pvField))
{
    init();
    Lock xx(globalMutex);
    totalConstruct++;
}

PVAuxInfo::~PVAuxInfo() {
    Lock xx(globalMutex);
    totalDestruct++;
    map_iterator i = pImpl->theMap.begin();
    while(i!=pImpl->theMap.end()) {
         PVScalar *value = i->second;
         delete value;
         i++;
    }
    delete pImpl;
}

PVField * PVAuxInfo::getPVField() {
    return pImpl->pvField;
}

ConstructDestructCallback *PVAuxInfo::getConstructDestructCallback()
{
    init();
    return pConstructDestructCallback;
}

PVScalar * PVAuxInfo::createInfo(String key,ScalarType scalarType)
{
    map_iterator i = pImpl->theMap.find(key);
    while(i!=pImpl->theMap.end()) {
        String message("AuxoInfo:create key ");
        message += key.c_str();
        message += " already exists with scalarType ";
        ScalarTypeFunc::toString(&message,scalarType);
        pImpl->pvField->message(message,errorMessage);
        i++;
    }
    PVScalar *pvScalar = getPVDataCreate()->createPVScalar(0,key,scalarType);
    pImpl->theMap.insert(std::pair<String,PVScalar * >(key, pvScalar));
    return pvScalar;

}

PVScalarMap PVAuxInfo::getInfos()
{
    return pImpl->theMap;
}

PVScalar * PVAuxInfo::getInfo(String key)
{
    map_iterator i = pImpl->theMap.find(key);
    if(i!=pImpl->theMap.end()) return i->second;
    return 0;
}

void PVAuxInfo::toString(StringBuilder buf)
{
    PVAuxInfo::toString(buf,0);
}

void PVAuxInfo::toString(StringBuilder buf,int indentLevel)
{
    Convert *convert = getConvert();
    convert->newLine(buf,indentLevel);
    *buf += "auxInfo";
    map_iterator i = pImpl->theMap.begin();
    while(i!=pImpl->theMap.end()) {
         convert->newLine(buf,indentLevel+1);
         String key = i->first;
         PVScalar *value = i->second;
         *buf += "key(";
         *buf += key.c_str();
         *buf += ") ";
         value->toString(buf,indentLevel + 1);
         i++;
    }
}
}}
