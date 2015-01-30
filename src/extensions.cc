#include <iostream>
#include <map>
#include <memory>
#include <string>

#include <node.h>
#include <nan.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlstring.h>
#include <libxslt/xslt.h>
#include <libxslt/extensions.h>

#include "./node_libxslt.h"
#include "./stylesheet.h"

#if 0
#define DBG(x) std::cerr << x << std::endl
#else
#define DBG(x)
#endif

class Module;

// Global variable used as registry
static std::map< std::basic_string<xmlChar>,
                 std::unique_ptr<Module> > moduleMap;

// Still looking for a more elegant way to identify the main event loop thread
static decltype(uv_thread_self()) main_thread;

static inline std::basic_string<xmlChar>
stringNode2xml(v8::Handle<v8::Value> obj) {
  v8::Local<v8::String> jstr = obj->ToString();
  size_t len = jstr->Utf8Length();
  std::unique_ptr<xmlChar[]> cstr(new xmlChar[len + 1]);
  jstr->WriteUtf8(reinterpret_cast<char*>(cstr.get()));
  return { cstr.get(), len };
}

static v8::Local<v8::Value> xpath2node(xmlXPathObjectPtr in) {
  switch (in->type) {
  case XPATH_BOOLEAN:
    return NanNew<v8::Boolean>(in->boolval);
  case XPATH_NUMBER:
    return NanNew<v8::Number>(in->floatval);
  case XPATH_STRING:
    return NanNew<v8::String>(in->stringval);
  default:
    NanThrowTypeError("XPath argument type not supported yet.");
    return NanNull();
  }
}

static xmlXPathObjectPtr node2xpath(v8::Handle<v8::Value> in) {
  if (in->IsBoolean() || in->IsBooleanObject())
    return xmlXPathNewBoolean(in->BooleanValue());
  if (in->IsNumber() || in->IsNumberObject())
    return xmlXPathNewFloat(in->NumberValue());
  std::basic_string<xmlChar> str = stringNode2xml(in);
  return xmlXPathNewString(str.c_str());
}

class Module {
public:
  std::map< std::basic_string<xmlChar>,
            std::unique_ptr<NanCallback> > functions;
};

// Compatibility function, since 59658a8 uv_thread_self() returns uv_thread_t
static inline bool uv_thread_equal(unsigned long a, unsigned long b) {
  return a == b;
}

class Context {
public:
  Context() {
    uv_async_init(uv_default_loop(), &async, evalFunction2);
    async.data = this;
    uv_mutex_init(&mtx);
  }  
  ~Context() {
    uv_mutex_destroy(&mtx);
  }
  void evalFunction1(xmlXPathParserContextPtr ctxt, int nargs) {
    this->ctxt = ctxt;
    argc = nargs;
    DBG("evalFunction1 called on " << uv_thread_self());
    if (uv_thread_equal(uv_thread_self(), main_thread)) {
      // synchroneous operation, so we can call back to v8 directly
      evalFunction3();
    }
    else {
      // called from worker thread, no v8 calls here please
      uv_mutex_lock(&mtx);
      uv_async_send(&async);
      // wait for evalFunction3 to unlock the mutex
      uv_mutex_lock(&mtx);
      // the value is already on the ctx stack, so we continue
      uv_mutex_unlock(&mtx);
    }
  }

private:
  uv_mutex_t mtx;
  uv_async_t async;
  xmlXPathParserContextPtr ctxt;
  int argc;

  static NAUV_WORK_CB(evalFunction2) {
    // called from main event loop
    Context *c = static_cast<Context*>(async->data);
    c->evalFunction3();
    uv_mutex_unlock(&c->mtx);
  }
  void evalFunction3() {
    // called from main event loop
    NanScope();
    xmlXPathContextPtr pctxt = ctxt->context;
    NanCallback& cb =
      *moduleMap.at(pctxt->functionURI)->functions.at(pctxt->function);
    std::unique_ptr<v8::Handle<v8::Value>[]> argv
      (new v8::Handle<v8::Value>[argc]);
    int i = argc;
    while (i > 0) {
      xmlXPathObjectPtr xobj = valuePop(ctxt);
      argv[--i] = xpath2node(xobj);
      xmlXPathFreeObject(xobj);
    }
    v8::Handle<v8::Value> res = cb.Call(argc, argv.get());
    valuePush(ctxt, node2xpath(res));
    uv_close(reinterpret_cast<uv_handle_t*>(&async), nullptr);
    // Now we signal the worker that the result is available
  }
};

static void evalFunction(xmlXPathParserContextPtr ctxt, int nargs) {
  // This is called on the XSLT thread, so we should not do Node stuff here.
  xsltTransformContextPtr tctxt = xsltXPathGetTransformContext(ctxt);
  Context *c = reinterpret_cast<Context*>
    (xsltGetExtData(tctxt, ctxt->context->functionURI));
  c->evalFunction1(ctxt, nargs);
}

static void* initFunc(xsltTransformContextPtr ctxt, const xmlChar *uri) {
  DBG("Context initialization started.");
  Module& module = *moduleMap.at(uri);
  for (auto i = module.functions.begin(), e = module.functions.end();
       i != e; ++i) {
    xsltRegisterExtFunction(ctxt, i->first.c_str(),
                            uri, evalFunction);
    DBG("Registering {" << (char*)uri << "}" << (char*)i->first.c_str());
  }
  Context *c = new Context();
  DBG("Context initialization finished.");
  return c;
}

static void shutdownFunc(xsltTransformContextPtr ctxt,
                         const xmlChar *uri, void *data) {
  Context *c = static_cast<Context*>(data);
  delete c;
}

NAN_METHOD(RegisterFunction) {
  NanScope();
  main_thread = uv_thread_self(); // let's hope this is the main thread.
  DBG("Main thread is " << main_thread);
  std::basic_string<xmlChar> name = stringNode2xml(args[0]);
  std::basic_string<xmlChar> uri = stringNode2xml(args[1]);
  std::unique_ptr<NanCallback> cb(new NanCallback(args[2].As<v8::Function>()));
  auto i = moduleMap.find(uri);
  if (i == moduleMap.end()) {
    i = moduleMap.emplace(uri, nullptr).first;
    i->second.reset(new Module());
    xsltRegisterExtModule(i->first.c_str(), initFunc, shutdownFunc);
    DBG("Registered module " << (char*)i->first.c_str());
  }
  Module& mod = *(i->second);
  mod.functions.emplace(name, std::move(cb));
  DBG("Registered {" << (char*)uri.c_str() << "}" << (char*)name.c_str());
  NanReturnUndefined();
}

NAN_METHOD(ShutdownOnExit) {
  DBG("Shutting down.");
  // We need to clear the moduleMap while V8 is still alive.
  // Otherwise the destructors of the persistent handles will segfault.
  // To avoid accidents, we unregister all our modules as well.
  NanScope();
  for (auto i = moduleMap.begin(), e = moduleMap.end(); i != e; ++i)
    xsltUnregisterExtModule(i->first.c_str());
  moduleMap.clear();
  NanReturnUndefined();
}
