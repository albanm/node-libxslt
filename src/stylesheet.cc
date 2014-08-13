#include <node.h>
#include <nan.h>

#include "./stylesheet.h"

using namespace v8;

Persistent<Function> Stylesheet::constructor;

Stylesheet::Stylesheet(xsltStylesheetPtr stylesheetPtr) : stylesheet_obj(stylesheetPtr) {}

Stylesheet::~Stylesheet()
{
    // TODO, potential memory leak here ?
    // We can't free the stylesheet as the xml doc inside was probably
    // already deleted by garbage collector and this results in segfaults
    //xsltFreeStylesheet(stylesheet_obj);
}

void Stylesheet::Init(Handle<Object> exports) {
	 // Prepare constructor template
  	Local<FunctionTemplate> tpl = FunctionTemplate::New();
  	tpl->SetClassName(String::NewSymbol("Stylesheet"));
  	tpl->InstanceTemplate()->SetInternalFieldCount(1);
  	
  	constructor = Persistent<Function>::New(tpl->GetFunction());
}

// not called from node, private api
Local<Object> Stylesheet::New(xsltStylesheetPtr stylesheetPtr) {
    NanEscapableScope();
    Local<Object> wrapper = NanNew(constructor)->NewInstance();
    Stylesheet* stylesheet = new Stylesheet(stylesheetPtr);
    stylesheet->Wrap(wrapper);
    return NanEscapeScope(wrapper);
}
