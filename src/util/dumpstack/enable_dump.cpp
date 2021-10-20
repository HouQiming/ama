#include "./dumpstack.h"

struct AutoEnableStackDump {
	AutoEnableStackDump() {DumpStack::EnableDump();}
};

static AutoEnableStackDump enabler{};
