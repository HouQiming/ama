#include <math.h>
#include "../src/script/jsapi.jch.hpp"

static JSValueConst JSerf(JSContext* ctx, JSValueConst this_val, int argc, JSValue* argv) {
	if(argc < 1){
		return JS_ThrowReferenceError(ctx,"need a number");
	}
	double val=0.0;
	JS_ToFloat64(ctx, &val, argv[0]);
	return JS_NewFloat64(ctx,erf(val));
}

extern "C" int AmaInit_native_module(JSValue module){
	JSValue exports=JS_GetPropertyStr(ama::jsctx, module, "exports");
	JS_SetPropertyStr(ama::jsctx, exports, "erf", JS_NewCFunction(ama::jsctx, JSerf, "erf", 1));
	return 0;
}
