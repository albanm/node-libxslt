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
#include "./extensions.h"

#if 0
#define DBG(x) std::cerr << x << std::endl
#else
#define DBG(x)
#endif

/* Here is how extensions work:
 *
 * The static initialization registers one namespace for us.
 * The code in other files creates a context.
 * The data for our namespace is retreived from that context.
 * This triggers initNodeXsltContext to allocate a new pointer.
 * The code in other places will fill that pointer appropriately,
 * creating a suitable (Async)NodeXsltContext.
 * We then have to register functions...
 *
 * When an xpath function ist to be evaluated, our code gets called.
 * We retrieve the NodeXsltContext and delegate to that.
 * This will ensure that callbacks are executed on the V8 thread.
 */

class Module;

// Global variable used as registry
static std::map< std::basic_string<xmlChar>, Module* > moduleMap;
typedef std::map< std::basic_string<xmlChar>, Module* >::iterator moduleMapIter;

static inline std::basic_string<xmlChar>
stringNode2xml(v8::Handle<v8::Value> obj) {
  v8::Local<v8::String> jstr = obj->ToString();
  size_t len = jstr->Utf8Length();
  xmlChar *cstr = new xmlChar[len + 1];
  jstr->WriteUtf8(reinterpret_cast<char*>(cstr));
  std::basic_string<xmlChar> str(cstr, len);
  delete[] cstr;
  return str;
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
  ~Module() {
    for (iter i = functions.begin(), e = functions.end();
         i != e; ++i) {
      delete i->second;
    }
  }
  std::map< std::basic_string<xmlChar>, NanCallback* > functions;
  typedef std::map< std::basic_string<xmlChar>, NanCallback* >::iterator iter;
};

const xmlChar* NodeXsltContext::NAMESPACE = (const xmlChar*)
  "https://github.com/albanm/node-libxslt";

void NodeXsltContext::dispose() {
  delete this;
}

// Synchroneous function evaluation, called from main event loop
void NodeXsltContext::evalFunction(xmlXPathParserContextPtr ctxt, int nargs) {
  DBG("evalFunction 5");
  NanScope();
  xmlXPathContextPtr pctxt = ctxt->context;
  NanCallback& cb =
    *moduleMap.at(pctxt->functionURI)->functions.at(pctxt->function);
  v8::Handle<v8::Value> *args = new v8::Handle<v8::Value>[nargs];
  int i = nargs;
  while (i > 0) {
    xmlXPathObjectPtr xobj = valuePop(ctxt);
    args[--i] = xpath2node(xobj);
    xmlXPathFreeObject(xobj);
  }
  v8::Handle<v8::Value> res = cb.Call(nargs, args);
  delete[] args;
  valuePush(ctxt, node2xpath(res));
}

class AsyncNodeXsltContext : public NodeXsltContext {
public:
  AsyncNodeXsltContext() {
    DBG("Creating async");
    uv_mutex_init(&mtx);
    uv_async_init(uv_default_loop(), &async, receiveAsync);
    async.data = this;
  }  
  ~AsyncNodeXsltContext() {
    DBG("Deleting context");
  }
  void dispose();
  void evalFunction(xmlXPathParserContextPtr ctxt, int nargs);

private:

  static NAUV_WORK_CB(receiveAsync);
  static void disposed(uv_handle_t* handle);
  void evalFunctionSync();

  uv_mutex_t mtx;
  uv_async_t async;
  xmlXPathParserContextPtr ctxt;
  int nargs;

};

NodeXsltContext* createContext(bool async) {
  if (async) return new AsyncNodeXsltContext();
  else return new NodeXsltContext();
}

void AsyncNodeXsltContext::dispose() {
  DBG("Closing async");
  uv_close(reinterpret_cast<uv_handle_t*>(&async), disposed);
  uv_mutex_destroy(&mtx);
}

void AsyncNodeXsltContext::disposed(uv_handle_t* handle) {
  void* data = reinterpret_cast<uv_async_t*>(handle)->data;
  delete static_cast<AsyncNodeXsltContext*>(data);
}

void AsyncNodeXsltContext::evalFunction(xmlXPathParserContextPtr ctxt, int nargs) {
  DBG("evalFunction 2");
  // called from worker thread, no v8 calls here please
  this->ctxt = ctxt;
  this->nargs = nargs;
  uv_mutex_lock(&mtx);
  DBG("got lock, sending");
  uv_async_send(&async);
  DBG("sent, waiting");
  uv_mutex_lock(&mtx); // block till evalFunctionSync is done
  DBG("received result, done");
  uv_mutex_unlock(&mtx); // result is already on the stack
}

inline void AsyncNodeXsltContext::evalFunctionSync() {
  DBG("evalFunction 4");
  NodeXsltContext::evalFunction(ctxt, nargs); // Call sync implementation
  DBG("have result, unlocking");
  uv_mutex_unlock(&mtx); // signal that we are done
}

NAUV_WORK_CB(AsyncNodeXsltContext::receiveAsync) {
  DBG("evalFunction 3");
  // Called from main event loop in response to async.
  // The mutex is locked at this point.
  static_cast<AsyncNodeXsltContext*>(async->data)->evalFunctionSync();
}

static void evalFunction(xmlXPathParserContextPtr ctxt, int nargs) {
  DBG("evalFunction 1");
  // This is called on the XSLT thread, so we should not do Node stuff here.
  xsltTransformContextPtr tctxt = xsltXPathGetTransformContext(ctxt);
  NodeXsltContext **c = reinterpret_cast<NodeXsltContext**>
    (xsltGetExtData(tctxt, NodeXsltContext::NAMESPACE));
  (**c).evalFunction(ctxt, nargs);
}

#if 0
static void* initFunc(xsltTransformContextPtr ctxt, const xmlChar *uri) {
  DBG("Context initialization started.");
  Module& module = *moduleMap.at(uri);
  for (Module::iter i = module.functions.begin(), e = module.functions.end();
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
#endif

NAN_METHOD(RegisterFunction) {
  NanScope();
  std::basic_string<xmlChar> name = stringNode2xml(args[0]);
  std::basic_string<xmlChar> uri = stringNode2xml(args[1]);
  NanCallback *cb = new NanCallback(args[2].As<v8::Function>());
  moduleMapIter i = moduleMap.find(uri);
  if (i == moduleMap.end()) {
    i = moduleMap.insert(std::make_pair(uri, new Module())).first;
    // xsltRegisterExtModule(i->first.c_str(), initFunc, shutdownFunc);
    DBG("Registered module " << (char*)i->first.c_str());
  }
  Module& mod = *(i->second);
  mod.functions.insert(std::make_pair(name, cb)); // mod assumes ownership of cb
  DBG("Registered {" << (char*)uri.c_str() << "}" << (char*)name.c_str());
  NanReturnUndefined();
}

static void* initNodeXsltContext(xsltTransformContextPtr ctxt, const xmlChar *uri) {
  DBG("Context initialization started.");
  for (moduleMapIter i = moduleMap.begin(), ei = moduleMap.end(); i != ei; ++i) {
    const xmlChar* uri = i->first.c_str();
    Module* module = i->second;
    for (Module::iter j = module->functions.begin(), ej = module->functions.end(); j != ej; ++j) {
      xsltRegisterExtFunction(ctxt, j->first.c_str(), uri, evalFunction);
      DBG("Registering {" << (char*)uri << "}" << (char*)j->first.c_str());
    }
  }
  DBG("Context initialization finished.");
  return new NodeXsltContext*();
}

static void shutdownNodeXsltContext(xsltTransformContextPtr ctxt,
                                    const xmlChar *uri, void *data) {
  NodeXsltContext** pnctxt = static_cast<NodeXsltContext**>(data);
  if (*pnctxt) (**pnctxt).dispose();
  delete pnctxt;
}

class Initialization { public: Initialization(); };
static Initialization runCodeUponLoading;
Initialization::Initialization() {
  xsltRegisterExtModule(NodeXsltContext::NAMESPACE,
                        initNodeXsltContext, shutdownNodeXsltContext);
}

NAN_METHOD(ShutdownOnExit) {
  DBG("Shutting down.");
  // We need to clear the moduleMap while V8 is still alive.
  // Otherwise the destructors of the persistent handles will segfault.
  // To avoid accidents, we unregister all our modules as well.
  NanScope();
  for (moduleMapIter i = moduleMap.begin(), e = moduleMap.end(); i != e; ++i) {
    xsltUnregisterExtModule(i->first.c_str());
    delete i->second;
  }
  moduleMap.clear();
  xsltUnregisterExtModule(NodeXsltContext::NAMESPACE);
  NanReturnUndefined();
}
