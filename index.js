var libxmljs = require('libxmljs');
//var binding = require('./build/Debug/node-libxslt');
var binding = require('bindings')('node-libxslt');

var Stylesheet = function(stylesheetDoc, stylesheetObj){
	// store both the source document and the parsed stylesheet
	// if we don't store the stylesheet doc it will be deleted by garbage collector and it will result in segfaults.
	this.stylesheetDoc = stylesheetDoc;
	this.stylesheetObj = stylesheetObj;
};
Stylesheet.prototype.apply = function(source, params, callback) {
	// params are optional
	if (typeof params === 'function') {
		callback = params;
		params = {};
	}
	params = params || {};

	for(var p in params) {
		// string parameters must be surrounded by quotes to be usable by the stylesheet
		if (typeof params[p] === 'string') params[p] = '\'' + params[p] + '\'';
	}

	// xml can be given as a string or a pre-parsed xml document
	var outputString = false;
	if (typeof source === 'string') {
		try {
			source = libxmljs.parseXml(source);
		} catch (err) {
			if (callback) return callback(err);
			throw err;
		}
		outputString = true;
	}

	// flatten the params object in an array
	var paramsArray = [];
	for(var key in params) {
		paramsArray.push(key);
		paramsArray.push(params[key]);
	}

	// for some obscure reason I didn't manage to create a new libxmljs document in applySync,
	// but passing a document by reference and modifying its content works fine
	var result = new libxmljs.Document();

	if (callback) {
		binding.applyAsync(this.stylesheetObj, source, paramsArray, result, function(err){
			if (err) return callback(err);
			callback(null, outputString ? result.toString() : result);
		});
	} else {
		binding.applySync(this.stylesheetObj, source, paramsArray, result);	
		return outputString ? result.toString() : result;
	}
};

exports.parse = function(source, callback) {
	// stylesheet can be given as a string or a pre-parsed xml document
	if (typeof source === 'string') source = libxmljs.parseXml(source);
	
	if (callback) {
		//return binding.stylesheetAsync(source);
	} else {
		return new Stylesheet(source, binding.stylesheetSync(source));
	}
};