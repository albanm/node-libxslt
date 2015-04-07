#define BUILDING_NODE_EXTENSION
#include <iostream>
#include <node.h>
#include <nan.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

// includes from libxmljs
#include <xml_syntax_error.h>
#include <xml_document.h>

#include "./node_libxslt.h"
#include "./stylesheet.h"

using namespace v8;

NAN_METHOD(StylesheetSync) {
  	NanScope();

    // From libxml document
    libxmljs::XmlDocument* doc = node::ObjectWrap::Unwrap<libxmljs::XmlDocument>(args[0]->ToObject());
    // From string
    //libxmljs::XmlDocument* doc = libxmljs::XmlDocument::FromXml(args);

    xsltStylesheetPtr stylesheet = xsltParseStylesheetDoc(doc->xml_obj);
    // TODO fetch actual error. 
    if (!stylesheet) {
        return NanThrowError("Could not parse XML string as XSLT stylesheet");
    }

    Local<Object> stylesheetWrapper = Stylesheet::New(stylesheet);
  	NanReturnValue(stylesheetWrapper);
}

// for memory the segfault i previously fixed were due to xml documents being deleted
// by garbage collector before their associated stylesheet.
class StylesheetWorker : public NanAsyncWorker {
 public:
  StylesheetWorker(libxmljs::XmlDocument* doc, NanCallback *callback)
    : NanAsyncWorker(callback), doc(doc) {}
  ~StylesheetWorker() {}

  // Executed inside the worker-thread.
  // It is not safe to access V8, or V8 data structures
  // here, so everything we need for input and output
  // should go on `this`.
  void Execute () {
    libxmljs::WorkerSentinel workerSentinel(workerParent);
    result = xsltParseStylesheetDoc(doc->xml_obj);
  }

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback () {
    NanScope();
    if (!result) {
        Local<Value> argv[] = { NanError("Failed to parse stylesheet") };
        callback->Call(2, argv);
    } else {
        Local<Object> resultWrapper = Stylesheet::New(result);
        Local<Value> argv[] = { NanNull(), resultWrapper };
        callback->Call(2, argv);
    }
  };

 private:
  libxmljs::WorkerParent workerParent;
  libxmljs::XmlDocument* doc;
  xsltStylesheetPtr result;
};

NAN_METHOD(StylesheetAsync) {
    NanScope();
    libxmljs::XmlDocument* doc = node::ObjectWrap::Unwrap<libxmljs::XmlDocument>(args[0]->ToObject());
    NanCallback *callback = new NanCallback(args[1].As<Function>());
    NanAsyncQueueWorker(new StylesheetWorker(doc, callback));
    NanReturnUndefined();
}

// duplicate from https://github.com/bsuh/node_xslt/blob/master/node_xslt.cc
void freeArray(char **array, int size) {
    for (int i = 0; i < size; i++) {
        free(array[i]);
    }
    free(array);
}
// transform a v8 array into a char** to pass params to xsl transform
// inspired by https://github.com/bsuh/node_xslt/blob/master/node_xslt.cc
char** PrepareParams(Handle<Array> array) {
    uint32_t arrayLen = array->Length();
    char** params = (char **)malloc(sizeof(char *) * (arrayLen + 1));
    memset(params, 0, sizeof(char *) * (array->Length() + 1));
    for (int i = 0; i < array->Length(); i++) {
        Local<String> param = array->Get(NanNew<Integer>(i))->ToString();
        params[i] = (char *)malloc(sizeof(char) * (param->Utf8Length() + 1));
        param->WriteUtf8(params[i]);
    }
    return params;
}

NAN_METHOD(ApplySync) {
    NanScope();

    Stylesheet* stylesheet = node::ObjectWrap::Unwrap<Stylesheet>(args[0]->ToObject());
    libxmljs::XmlDocument* docSource = node::ObjectWrap::Unwrap<libxmljs::XmlDocument>(args[1]->ToObject());
    Handle<Array> paramsArray = Handle<Array>::Cast(args[2]);
    libxmljs::XmlDocument* docResult = node::ObjectWrap::Unwrap<libxmljs::XmlDocument>(args[3]->ToObject());

    char** params = PrepareParams(paramsArray);

    xmlDoc* result = xsltApplyStylesheet(stylesheet->stylesheet_obj, docSource->xml_obj, (const char **)params);
    if (!result) {
        freeArray(params, paramsArray->Length());
        return NanThrowError("Failed to apply stylesheet");
    }

    // for some obscure reason I didn't manage to create a new libxmljs document in applySync,
	// but passing a document by reference and modifying its content works fine
    // replace the empty document in docResult with the result of the stylesheet
	docResult->xml_obj->_private = NULL;
    xmlFreeDoc(docResult->xml_obj);
    docResult->xml_obj = result;
    result->_private = docResult;

    freeArray(params, paramsArray->Length());

  	NanReturnUndefined();
}

// for memory the segfault i previously fixed were due to xml documents being deleted
// by garbage collector before their associated stylesheet.
class ApplyWorker : public NanAsyncWorker {
 public:
  ApplyWorker(Stylesheet* stylesheet, libxmljs::XmlDocument* docSource, char** params, int paramsLength, libxmljs::XmlDocument* docResult, NanCallback *callback)
    : NanAsyncWorker(callback), stylesheet(stylesheet), docSource(docSource), params(params), paramsLength(paramsLength), docResult(docResult) {}
  ~ApplyWorker() {}

  // Executed inside the worker-thread.
  // It is not safe to access V8, or V8 data structures
  // here, so everything we need for input and output
  // should go on `this`.
  void Execute () {
    libxmljs::WorkerSentinel workerSentinel(workerParent);
    result = xsltApplyStylesheet(stylesheet->stylesheet_obj, docSource->xml_obj, (const char **)params);
  }

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback () {
    NanScope();

    if (!result) {
        Local<Value> argv[] = { NanError("Failed to apply stylesheet") };
        freeArray(params, paramsLength);
        callback->Call(2, argv);
    } else {
        Local<Value> argv[] = { NanNull() };

        // for some obscure reason I didn't manage to create a new libxmljs document in applySync,
        // but passing a document by reference and modifying its content works fine
        // replace the empty document in docResult with the result of the stylesheet
        docResult->xml_obj->_private = NULL;
        xmlFreeDoc(docResult->xml_obj);
        docResult->xml_obj = result;
        result->_private = docResult;

        freeArray(params, paramsLength);
    
        callback->Call(1, argv);
    }
  };

 private:
  libxmljs::WorkerParent workerParent;
  Stylesheet* stylesheet;
  libxmljs::XmlDocument* docSource;
  char** params;
  int paramsLength;
  libxmljs::XmlDocument* docResult;
  xmlDoc* result;
};

NAN_METHOD(ApplyAsync) {
    NanScope();

    Stylesheet* stylesheet = node::ObjectWrap::Unwrap<Stylesheet>(args[0]->ToObject());
    libxmljs::XmlDocument* docSource = node::ObjectWrap::Unwrap<libxmljs::XmlDocument>(args[1]->ToObject());
    Handle<Array> paramsArray = Handle<Array>::Cast(args[2]);
    libxmljs::XmlDocument* docResult = node::ObjectWrap::Unwrap<libxmljs::XmlDocument>(args[3]->ToObject());
    NanCallback *callback = new NanCallback(args[4].As<Function>());

    char** params = PrepareParams(paramsArray);

    NanAsyncQueueWorker(new ApplyWorker(stylesheet, docSource, params, paramsArray->Length(), docResult, callback));
    NanReturnUndefined();
}

NAN_METHOD(RegisterEXSLT) {
    exsltRegisterAll();
    NanReturnUndefined();
}

// Compose the module by assigning the methods previously prepared
void InitAll(Handle<Object> exports) {
  	Stylesheet::Init(exports);
  	exports->Set(NanNew<String>("stylesheetSync"), NanNew<FunctionTemplate>(StylesheetSync)->GetFunction());
    exports->Set(NanNew<String>("stylesheetAsync"), NanNew<FunctionTemplate>(StylesheetAsync)->GetFunction());
  	exports->Set(NanNew<String>("applySync"), NanNew<FunctionTemplate>(ApplySync)->GetFunction());
    exports->Set(NanNew<String>("applyAsync"), NanNew<FunctionTemplate>(ApplyAsync)->GetFunction());
    exports->Set(NanNew<String>("registerEXSLT"), NanNew<FunctionTemplate>(RegisterEXSLT)->GetFunction());
}
NODE_MODULE(node_libxslt, InitAll);
