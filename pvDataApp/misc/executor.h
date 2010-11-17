/* executor.h */
#ifndef EXECUTOR_H
#define EXECUTOR_H
#include <memory>
#include <vector>
#include "noDefaultMethods.h"
#include "pvType.h"
#include "thread.h"

namespace epics { namespace pvData { 

// This is created by Executor.createNode and passed to Executor.execute
class ExecutorNode;

class Command {
public:
    virtual void command() = 0;
};

class Executor : private NoDefaultMethods {
public:
    Executor(String threadName,ThreadPriority priority);
    static ConstructDestructCallback *getConstructDestructCallback();
    ExecutorNode * createNode(Command *command);
    void execute(ExecutorNode *node);
    void destroy();
private:
    ~Executor();
    class ExecutorPvt *pImpl;
};

}}
#endif  /* EXECUTOR_H */
