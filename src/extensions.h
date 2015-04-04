class NodeXsltContext {
public:
  virtual ~NodeXsltContext() { }
  virtual void dispose();
  virtual void evalFunction(xmlXPathParserContextPtr ctxt, int nargs);
  static const xmlChar* NAMESPACE;
};

NodeXsltContext* createContext(bool async);
