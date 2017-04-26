#ifndef DCRead_H
#define DCRead_H
#include <uv.h>
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include "dcrf32.h"

class DCRead : public node::ObjectWrap {
public:
	static void Init(v8::Handle<v8::Object> exports);

private:
	explicit DCRead();
	~DCRead();

	static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void DC_Start_RealReadCard(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void DC_Stop_RealReadCard(const v8::FunctionCallbackInfo<v8::Value>& args);
	static v8::Persistent<v8::Function> constructor;
};

#endif
