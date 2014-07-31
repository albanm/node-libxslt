#define BUILDING_NODE_EXTENSION
#include <iostream>
#include <node.h>
#include <nan.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>

// includes from libxmljs
#include <xml_syntax_error.h>
#include <xml_document.h>

#include "./node_libxslt.h"
#include "./stylesheet.h"

using namespace v8;

NAN_METHOD(StylesheetSync) {
	std::cout << "StylesheetSync!" << std::endl;

  	NanScope();

    libxmljs::XmlDocument* doc = node::ObjectWrap::Unwrap<libxmljs::XmlDocument>(args[0]->ToObject());

    xsltStylesheetPtr stylesheet = xsltParseStylesheetDoc(doc->xml_obj);
    // TODO fetch actual error. 
    if (!stylesheet) {
        return NanThrowError("Could not parse XML string as XSLT stylesheet");
    }

	Local<Object> stylesheetWrapper = Stylesheet::New(stylesheet);
  	NanReturnValue(stylesheetWrapper);
}

NAN_METHOD(ApplySync) {
    std::cout << "ApplySync!" << std::endl;

  	NanScope();

    Stylesheet* stylesheet = node::ObjectWrap::Unwrap<Stylesheet>(args[0]->ToObject());
    libxmljs::XmlDocument* docSource = node::ObjectWrap::Unwrap<libxmljs::XmlDocument>(args[1]->ToObject());
    Handle<Array> array = Handle<Array>::Cast(args[2]);
    libxmljs::XmlDocument* docResult = node::ObjectWrap::Unwrap<libxmljs::XmlDocument>(args[3]->ToObject());

    // transform a v8 array into a char** to pass params to xsl transform
    // inspired by https://github.com/bsuh/node_xslt/blob/master/node_xslt.cc
    uint32_t arrayLen = array->Length();
    if (arrayLen % 2 != 0) {
        return NanThrowError("Array contains an odd number of parameters");
    }
    char** params = (char **)malloc(sizeof(char *) * (arrayLen + 1));
    if (!params) {
        return NanThrowError("Failed to allocate memory");
    }
    memset(params, 0, sizeof(char *) * (array->Length() + 1));
    for (int i = 0; i < array->Length(); i++) {
        Local<String> param = array->Get(NanNew<Integer>(i))->ToString();
        params[i] = (char *)malloc(sizeof(char) * (param->Length() + 1));
        if (!params[i]) {
            return NanThrowError("Failed to allocate memory");
        }
        param->WriteAscii(params[i]);
    }

    xmlDoc* result = xsltApplyStylesheet(stylesheet->stylesheet_obj, docSource->xml_obj, (const char **)params);
    if (!result) {
        return NanThrowError("Failed to apply stylesheet");
    }

    // for some obscure reason I didn't manage to create a new libxmljs document in applySync,
	// but passing a document by reference and modifying its content works fine
    // replace the empty document in docResult with the result of the stylesheet
	docResult->xml_obj->_private = NULL;
    //xmlFreeDoc(docResult->xml_obj);
    docResult->xml_obj = result;
    result->_private = docResult;

  	NanReturnUndefined();
}

// Compose the module by assigning the methods previously prepared
void InitAll(Handle<Object> exports) {
  	Stylesheet::Init(exports);
  	exports->Set(NanNew<String>("stylesheetSync"), NanNew<FunctionTemplate>(StylesheetSync)->GetFunction());
  	exports->Set(NanNew<String>("applySync"), NanNew<FunctionTemplate>(ApplySync)->GetFunction());
}
NODE_MODULE(node_libxslt, InitAll);