#include <node.h>
#include <nan.h>

#include "./document.h"

using namespace v8;

Persistent<Function> Document::constructor;

Document::Document(xmlDocumentPtr documentPtr) : document_obj(documentPtr) {}

Document::~Document()
{
    xmlFreeDocument(document_obj);
}

void Document::Init(Handle<Object> exports) {
	 // Prepare constructor template
  	Local<FunctionTemplate> tpl = FunctionTemplate::New();
  	tpl->SetClassName(String::NewSymbol("Document"));
  	tpl->InstanceTemplate()->SetInternalFieldCount(1);
  	
  	constructor = Persistent<Function>::New(tpl->GetFunction());
}

// not called from node, private api
Local<Object> Document::New(xmlDocumentPtr documentPtr) {
    NanEscapableScope();
    Local<Object> wrapper = NanNew(constructor)->NewInstance();
    Document* Document = new Document(documentPtr);
    Document->Wrap(wrapper);
    return NanEscapeScope(wrapper);
}
