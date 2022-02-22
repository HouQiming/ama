#include "./dumpstack.h"
#pragma no_auto_header()

struct AutoEnableStackDump {
	AutoEnableStackDump() {DumpStack::EnableDump();}
};

static AutoEnableStackDump enabler{};
