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

//int vasprintf (char **strp, const char *fmt, va_list ap);

        int vasprintf(char** strp, const char* fmt, va_list ap) {
            va_list ap2;
            va_copy(ap2, ap);
            char tmp[1];
            int size = vsnprintf(tmp, 1, fmt, ap2);
            if (size <= 0) return size;
            va_end(ap2);
            size += 1;
            *strp = (char*)malloc(size * sizeof(char));
            return vsnprintf(*strp, size, fmt, ap);
        }

static xmlDoc* copyDocument(Local<Value> input) {
    libxmljs::XmlDocument* docWrapper =
        Nan::ObjectWrap::Unwrap<libxmljs::XmlDocument>(Nan::To<v8::Object>(input).ToLocalChecked());
    xmlDoc* stylesheetDoc = docWrapper->xml_obj;
    return xmlCopyDoc(stylesheetDoc, true);
}

// Directly inspired by nokogiri:
// https://github.com/sparklemotion/nokogiri/blob/24bb843327306d2d71e4b2dc337c1e327cbf4516/ext/nokogiri/xslt_stylesheet.c#L76
static void xslt_generic_error_handler(void * ctx, const char *msg, ...)
{
  char * message;
  va_list args;
  va_start(args, msg);
  vasprintf(&message, msg, args);
  va_end(args);
  strncpy((char*)ctx, message, 2048);
  free(message);
}

NAN_METHOD(StylesheetSync) {
    Nan::HandleScope scope;
    char* errstr = new char[2048];
    xsltSetGenericErrorFunc(errstr, xslt_generic_error_handler);
    xmlDoc* doc = copyDocument(info[0]);
    xsltStylesheetPtr stylesheet = xsltParseStylesheetDoc(doc);
    xsltSetGenericErrorFunc(NULL, NULL);

    if (!stylesheet) {
        xmlFreeDoc(doc);
        return Nan::ThrowError(errstr);
    }

    Local<Object> stylesheetWrapper = Stylesheet::New(stylesheet);
  	info.GetReturnValue().Set(stylesheetWrapper);
}

// for memory the segfault i previously fixed were due to xml documents being deleted
// by garbage collector before their associated stylesheet.
class StylesheetWorker : public Nan::AsyncWorker {
 public:
  StylesheetWorker(xmlDoc* doc, Nan::Callback *callback)
    : Nan::AsyncWorker(callback), doc(doc) {}
  ~StylesheetWorker() {}

  // Executed inside the worker-thread.
  // It is not safe to access V8, or V8 data structures
  // here, so everything we need for input and output
  // should go on `this`.
  void Execute () {
    libxmljs::WorkerSentinel workerSentinel(workerParent);

    // Error management is probably not really thread safe :(
    errstr = new char[2048];;
    xsltSetGenericErrorFunc(errstr, xslt_generic_error_handler);
    result = xsltParseStylesheetDoc(doc);
    xsltSetGenericErrorFunc(NULL, NULL);
  }

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback () {
    Nan::HandleScope scope;
    if (!result) {
        xmlFreeDoc(doc);
        Local<Value> argv[] = { Nan::Error(errstr) };
        callback->Call(1, argv);
    } else {
        Local<Object> resultWrapper = Stylesheet::New(result);
        Local<Value> argv[] = { Nan::Null(), resultWrapper };
        callback->Call(2, argv);
    }
  };

 private:
  libxmljs::WorkerParent workerParent;
  xmlDoc* doc;
  xsltStylesheetPtr result;
  char* errstr;
};

NAN_METHOD(StylesheetAsync) {
    Nan::HandleScope scope;
    xmlDoc* doc = copyDocument(info[0]);
    Nan::Callback *callback = new Nan::Callback(info[1].As<Function>());
    StylesheetWorker* worker = new StylesheetWorker(doc, callback);
    Nan::AsyncQueueWorker(worker);
    return;
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
char** PrepareParams(Local<Array> array, Isolate *isolate) {
    uint32_t arrayLen = array->Length();
    char** params = (char **)malloc(sizeof(char *) * (arrayLen + 1));
    memset(params, 0, sizeof(char *) * (array->Length() + 1));
    for (unsigned int i = 0; i < array->Length(); i++) {
        Local<String> param = Nan::To<v8::String>(Nan::Get(array, i).ToLocalChecked()).ToLocalChecked();
        params[i] = (char *)malloc(sizeof(char) * (param->Utf8Length(isolate) + 1));
        param->WriteUtf8(isolate, params[i]);
    }
    return params;
}

NAN_METHOD(ApplySync) {
    Nan::HandleScope scope;
    Stylesheet* stylesheet = Nan::ObjectWrap::Unwrap<Stylesheet>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
    libxmljs::XmlDocument* docSource = Nan::ObjectWrap::Unwrap<libxmljs::XmlDocument>(Nan::To<v8::Object>(info[1]).ToLocalChecked());
    Local<Array> paramsArray = Local<Array>::Cast(info[2]);
    bool outputString = Nan::To<v8::Boolean>(info[3]).ToLocalChecked()->Value();

    char** params = PrepareParams(paramsArray, info.GetIsolate());

    xmlDoc* result = xsltApplyStylesheet(stylesheet->stylesheet_obj, docSource->xml_obj, (const char **)params);
    if (!result) {
        freeArray(params, paramsArray->Length());
        return Nan::ThrowError("Failed to apply stylesheet");
    }

    if (outputString) {
      // As well as a libxmljs document, prepare a string result
      unsigned char* resStr;
      int len;
      xsltSaveResultToString(&resStr,&len,result,stylesheet->stylesheet_obj);
      xmlFreeDoc(result);
      info.GetReturnValue().Set(Nan::New<String>(resStr ? (char*)resStr : "").ToLocalChecked());
      if (resStr) xmlFree(resStr);
    } else {
      // Fill a result libxmljs document.
      // for some obscure reason I didn't manage to create a new libxmljs document in applySync,
    	// but passing a document by reference and modifying its content works fine
      // replace the empty document in docResult with the result of the stylesheet
      libxmljs::XmlDocument* docResult = Nan::ObjectWrap::Unwrap<libxmljs::XmlDocument>(Nan::To<v8::Object>(info[4]).ToLocalChecked());
      docResult->xml_obj->_private = NULL;
      xmlFreeDoc(docResult->xml_obj);
      docResult->xml_obj = result;
      result->_private = docResult;
    }

    freeArray(params, paramsArray->Length());
  	return;
}

// for memory the segfault i previously fixed were due to xml documents being deleted
// by garbage collector before their associated stylesheet.
class ApplyWorker : public Nan::AsyncWorker {
 public:
  ApplyWorker(Stylesheet* stylesheet, libxmljs::XmlDocument* docSource, char** params, int paramsLength, bool outputString, libxmljs::XmlDocument* docResult, Nan::Callback *callback)
    : Nan::AsyncWorker(callback), stylesheet(stylesheet), docSource(docSource), params(params), paramsLength(paramsLength), outputString(outputString), docResult(docResult) {}
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
    Nan::HandleScope scope;
    if (!result) {
        Local<Value> argv[] = { Nan::Error("Failed to apply stylesheet") };
        freeArray(params, paramsLength);
        callback->Call(1, argv);
        return;
    }

    if(!outputString) {
      // for some obscure reason I didn't manage to create a new libxmljs document in applySync,
      // but passing a document by reference and modifying its content works fine
      // replace the empty document in docResult with the result of the stylesheet
      docResult->xml_obj->_private = NULL;
      xmlFreeDoc(docResult->xml_obj);
      docResult->xml_obj = result;
      result->_private = docResult;
      Local<Value> argv[] = { Nan::Null() };
      freeArray(params, paramsLength);
      callback->Call(1, argv);
    } else {
      unsigned char* resStr;
      int len;
      int cnt=xsltSaveResultToString(&resStr,&len,result,stylesheet->stylesheet_obj);
      xmlFreeDoc(result);
      Local<Value> argv[] = { Nan::Null(), Nan::New<String>(resStr ? (char*)resStr : "").ToLocalChecked()};
      if (resStr) xmlFree(resStr);
      freeArray(params, paramsLength);
      callback->Call(2, argv);
    }


  };

 private:
  libxmljs::WorkerParent workerParent;
  Stylesheet* stylesheet;
  libxmljs::XmlDocument* docSource;
  char** params;
  int paramsLength;
  bool outputString;
  libxmljs::XmlDocument* docResult;
  xmlDoc* result;
};

NAN_METHOD(ApplyAsync) {
    Nan::HandleScope scope;

    Stylesheet* stylesheet = Nan::ObjectWrap::Unwrap<Stylesheet>(Nan::To<v8::Object>(info[0]).ToLocalChecked());
    libxmljs::XmlDocument* docSource = Nan::ObjectWrap::Unwrap<libxmljs::XmlDocument>(Nan::To<v8::Object>(info[1]).ToLocalChecked());
    Local<Array> paramsArray = Local<Array>::Cast(info[2]);
    bool outputString = Nan::To<v8::Boolean>(info[3]).ToLocalChecked()->Value();

    //if (!outputString) {
    libxmljs::XmlDocument* docResult = Nan::ObjectWrap::Unwrap<libxmljs::XmlDocument>(Nan::To<v8::Object>(info[4]).ToLocalChecked());
    //}

    Nan::Callback *callback = new Nan::Callback(info[5].As<Function>());

    char** params = PrepareParams(paramsArray, info.GetIsolate());

    ApplyWorker* worker = new ApplyWorker(stylesheet, docSource, params, paramsArray->Length(), outputString, docResult, callback);
    for (uint32_t i = 0; i < 5; ++i) worker->SaveToPersistent(i, info[i]);
    Nan::AsyncQueueWorker(worker);
    return;
}

NAN_METHOD(RegisterEXSLT) {
    exsltRegisterAll();
    return;
}

// Compose the module by assigning the methods previously prepared
void InitAll(Local<Object> exports) {
  	Stylesheet::Init(exports);
    Nan::Set(exports, Nan::New("stylesheetSync").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(StylesheetSync)).ToLocalChecked());
    Nan::Set(exports, Nan::New("stylesheetAsync").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(StylesheetAsync)).ToLocalChecked());
    Nan::Set(exports, Nan::New("applySync").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(ApplySync)).ToLocalChecked());
    Nan::Set(exports, Nan::New("applyAsync").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(ApplyAsync)).ToLocalChecked());
    Nan::Set(exports, Nan::New("registerEXSLT").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(RegisterEXSLT)).ToLocalChecked());
}
NODE_MODULE(node_libxslt, InitAll);
