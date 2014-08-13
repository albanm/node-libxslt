node-libxslt
============

Node.js bindings for [libxslt](http://xmlsoft.org/libxslt/) compatible with [libxmljs](https://github.com/polotek/libxmljs/issues/226).

Installation
------------

	npm install node-libxslt

Basic usage
-----------

```js
var lixslt = require('node-libxslt');

var stylesheet = libxslt.stylesheet(stylesheetString);

var params = {
	MyParam: 'my value'
};

// 'params' parameter is optional
stylesheet.apply(documentString, params, function(err, result){
	// err contains any error from parsing the document or applying the the stylesheet
	// result is a string containing the result of the transformation
});

```

Libxmljs integration
--------------------

Node-libxslt depends on [libxmljs](https://github.com/polotek/libxmljs/issues/226) in the same way that [libxslt](http://xmlsoft.org/libxslt/) depends on [libxml](http://xmlsoft.org/). This dependancy makes possible to bundle and to load in memory libxml only once for users of both libraries.

It is possible to work with libxmljs documents instead of strings:

```js
var lixslt = require('node-libxslt');
var libxmljs = require('libxmljs');

var stylesheetObj = libxmljs.parseXml(stylesheetString);
var stylesheet = libxslt.stylesheet(stylesheetObj);

var document = libxmljs.parseXml(documentString);
stylesheet.apply(document, function(err, result){
	// result is now a libxmljs document containing the result of the transformation
});

```

This is only useful if you already needed to parse a document before applying the stylesheet for previous manipulations.
Or if you wish to be returned a document instead of a string for ulterior manipulations.
In these cases you will prevent extraneous parsings and serializations.	

Sync or async
-------------

The same *apply()* function can be used in synchronous mode simply by removing the callback parameter.
In this case if a parsing error occurs it will be thrown.

```js
var lixslt = require('node-libxslt');

var stylesheet = libxslt.stylesheet(stylesheetString);

var result = stylesheet.apply(documentString);

```

The asynchronous function uses the [libuv work queue](http://nikhilm.github.io/uvbook/threads.html#libuv-work-queue)
to provide parallelized computation in node.js worker threads. This makes it non-blocking for the main event loop of node.js.

Note that libxmljs parsing doesn't use the work queue, so only a part of the process is actually parallelized.

A small benchmark is available in the project. It has a very limited scope, it uses always the same small transformation a few thousand times.
To run it use:

    node benchmark.js

This is an example of its results with an intel core i5 3.1GHz:

```
10000 synchronous apply from parsed doc			 		in 331ms = 30211/s
10000 asynchronous apply in series from parsed doc		in 538ms = 18587/s
10000 asynchronous apply in parallel from parsed doc	in 217ms = 46083/s
```

Observations:
  - it's pretty fast !
  - asynchronous is slower when running in series.
  - asynchronous can become faster when concurrency is high.

Conclusion:
  - use asynchronous by default it will be kinder to your main event loop and is pretty fast anyway.
  - use synchronous only if you really want the highest performance and expect low concurrency.
  - of course you can also use synchronous simply to reduce code depth. If you don't expect a huge load it will be ok.

OS compatibility
----------------

Right now only 64bits linux is supported. The support of other environments is a work in progress.

For windows user: node-libxslt depends on [node-gyp](https://github.com/TooTallNate/node-gyp), you will have to go through its installation.